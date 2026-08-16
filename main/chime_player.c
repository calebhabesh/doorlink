#include "chime_player.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "board_pins.h"
#include "audio_bus.h"
#include "doorbell_chime_pcm.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "speaker_output.h"
#include "sdkconfig.h"

#define CHIME_SAMPLE_RATE_HZ 44100
#define CHIME_WRITE_CHUNK_BYTES 1024

#ifndef CONFIG_SMART_DOORBELL_CAMERA_OVERLAP_ACK
#define CONFIG_SMART_DOORBELL_CAMERA_OVERLAP_ACK 0
#endif
#ifndef CONFIG_SMART_DOORBELL_CAMERA_OVERLAP_ACK_ATTENUATION_DB
#define CONFIG_SMART_DOORBELL_CAMERA_OVERLAP_ACK_ATTENUATION_DB 12
#endif
#ifndef CONFIG_SMART_DOORBELL_CAMERA_OVERLAP_ACK_DURATION_MS
#define CONFIG_SMART_DOORBELL_CAMERA_OVERLAP_ACK_DURATION_MS 3000
#endif

static const char *TAG = "chime_player";
static atomic_bool s_is_playing = false;
static atomic_bool s_retrigger_requested = false;
static atomic_bool s_stop_requested = false;
static atomic_bool s_interruptible = false;
static atomic_bool s_camera_overlap_mode = false;
static atomic_bool s_tearing_down = false;
static atomic_bool s_restart_after_teardown = false;
static atomic_bool s_low_power_overlap = false;
static atomic_bool s_output_active = false;
static portMUX_TYPE s_claim_lock = portMUX_INITIALIZER_UNLOCKED;

static void release_playback_claim(void)
{
    atomic_store(&s_retrigger_requested, false);
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_interruptible, false);
    atomic_store(&s_tearing_down, false);
    atomic_store(&s_restart_after_teardown, false);
    atomic_store(&s_low_power_overlap, false);
    atomic_store(&s_output_active, false);
    // Publish idle last so a new caller cannot claim playback while the old
    // task is still resetting shared state.
    atomic_store(&s_is_playing, false);
}

static esp_err_t init_speaker_i2s(i2s_chan_handle_t *tx_chan)
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    esp_err_t err = i2s_new_channel(&chan_cfg, tx_chan, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(CHIME_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_SCK,
            .ws = I2S_AUDIO_WS,
            .dout = I2S_SPK_SD,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

    err = i2s_channel_init_std_mode(*tx_chan, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s",
                 esp_err_to_name(err));
        i2s_del_channel(*tx_chan);
        *tx_chan = NULL;
        return err;
    }

    err = i2s_channel_enable(*tx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(err));
        i2s_del_channel(*tx_chan);
        *tx_chan = NULL;
        return err;
    }

    return ESP_OK;
}

static esp_err_t play_claimed_chime(TickType_t bus_timeout_ticks,
                                    size_t playback_len,
                                    unsigned attenuation_db,
                                    unsigned ramp_ms,
                                    bool allow_retrigger,
                                    const char *description)
{
restart_playback:
    if (!audio_bus_acquire(bus_timeout_ticks)) {
        if (bus_timeout_ticks == 0) {
            ESP_LOGI(TAG,
                     "Repress chime skipped because visitor/homeowner audio owns I2S");
        } else {
            ESP_LOGE(TAG, "Could not acquire I2S for first local chime");
        }
        release_playback_claim();
        return ESP_ERR_TIMEOUT;
    }

    if (atomic_load(&s_stop_requested)) {
        audio_bus_release();
        release_playback_claim();
        return ESP_OK;
    }

    // Configure AMP_EN_PIN as output and enable MAX98357A amp
    gpio_reset_pin(AMP_EN_PIN);
    gpio_set_direction(AMP_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(AMP_EN_PIN, 1);
    atomic_store(&s_output_active, true);
    vTaskDelay(pdMS_TO_TICKS(10)); // Allow amp power-on settle

    i2s_chan_handle_t tx_chan = NULL;
    esp_err_t err = init_speaker_i2s(&tx_chan);
    if (err != ESP_OK) {
        gpio_set_level(AMP_EN_PIN, 0);
        audio_bus_release();
        release_playback_claim();
        return err;
    }

    if (playback_len > g_doorbell_chime_pcm_len) {
        playback_len = g_doorbell_chime_pcm_len;
    }
    playback_len &= ~(size_t)3U;
    ESP_LOGI(TAG,
             "Playing %s (%u bytes PCM @ 44.1kHz stereo, attenuation=-%u dB ramp=%u ms)",
             description, (unsigned)playback_len, attenuation_db, ramp_ms);

    size_t offset = 0;
    size_t bytes_written = 0;
    int16_t scaled_samples[CHIME_WRITE_CHUNK_BYTES / sizeof(int16_t)];
    speaker_envelope_t envelope;
    speaker_envelope_init_profile(&envelope, CHIME_SAMPLE_RATE_HZ,
                                  playback_len / 4U, attenuation_db, ramp_ms);

stream_live_channel:
    while (offset < playback_len) {
        if (atomic_load(&s_stop_requested)) {
            ESP_LOGI(TAG, "Local chime yielded to higher-priority audio");
            break;
        }
        if (allow_retrigger &&
            atomic_exchange(&s_retrigger_requested, false)) {
            // Keep the amplifier and I2S channel live. Rewinding at a 1 KiB
            // PCM boundary acknowledges a new press without task creation,
            // bus reacquisition, or an audible mute/teardown gap.
            offset = 0;
            ESP_LOGI(TAG,
                     "Button repress retriggered local chime from sample 0");
        }

        size_t chunk_len = playback_len - offset;
        if (chunk_len > CHIME_WRITE_CHUNK_BYTES) {
            chunk_len = CHIME_WRITE_CHUNK_BYTES;
        }

        const size_t sample_count = chunk_len / sizeof(int16_t);
        for (size_t sample_index = 0; sample_index < sample_count;
             ++sample_index) {
            const size_t byte_index = offset + sample_index * 2U;
            const uint16_t raw =
                (uint16_t)g_doorbell_chime_pcm[byte_index] |
                ((uint16_t)g_doorbell_chime_pcm[byte_index + 1U] << 8U);
            const int16_t sample = (int16_t)raw;
            const size_t frame_index = offset / 4U + sample_index / 2U;
            scaled_samples[sample_index] =
                speaker_envelope_apply(&envelope, sample, frame_index);
        }

        err = i2s_channel_write(tx_chan, scaled_samples,
                                chunk_len, &bytes_written, pdMS_TO_TICKS(100));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "I2S write failed at offset %u: %s", (unsigned)offset,
                     esp_err_to_name(err));
            break;
        }
        offset += bytes_written;
    }

    // Flush with silence and wait for DMA to drain to avoid click/pop or repeating buffer noise
    int16_t silence[256 * 2] = {0};
    i2s_channel_write(tx_chan, silence, sizeof(silence), &bytes_written,
                      pdMS_TO_TICKS(100));
    vTaskDelay(pdMS_TO_TICKS(50)); // Allow hardware DMA buffer to complete playback

    // A down-edge may arrive after the stream's final chunk but before the
    // amplifier/I2S teardown completes. Consume that edge while the live
    // channel is still available; otherwise it would be acknowledged by the
    // caller and then silently discarded during teardown.
    bool rewind_before_teardown = false;
    portENTER_CRITICAL(&s_claim_lock);
    if (allow_retrigger && err == ESP_OK &&
        !atomic_load(&s_stop_requested) &&
        atomic_exchange(&s_retrigger_requested, false)) {
        rewind_before_teardown = true;
    } else {
        atomic_store(&s_tearing_down, true);
    }
    portEXIT_CRITICAL(&s_claim_lock);

    if (rewind_before_teardown) {
        ESP_LOGI(TAG, "Late button repress reused the live I2S channel");
        offset = 0;
        goto stream_live_channel;
    }

    // Mute amp first, then tear down I2S channel
    atomic_store(&s_output_active, false);
    gpio_set_level(AMP_EN_PIN, 0);
    i2s_channel_disable(tx_chan);
    i2s_del_channel(tx_chan);

    ESP_LOGI(TAG, "Local doorbell chime playback completed cleanly");
    audio_bus_release();

    bool restart_after_teardown = false;
    portENTER_CRITICAL(&s_claim_lock);
    if (allow_retrigger && err == ESP_OK &&
        !atomic_load(&s_stop_requested) &&
        atomic_exchange(&s_restart_after_teardown, false)) {
        atomic_store(&s_tearing_down, false);
        atomic_store(&s_interruptible, true);
        restart_after_teardown = true;
    } else {
        // Publish IDLE while holding the same lock used by claimers. A press
        // can now either request this task's restart or become the next owner;
        // it cannot land between the final restart check and IDLE publication.
        release_playback_claim();
    }
    portEXIT_CRITICAL(&s_claim_lock);

    if (restart_after_teardown) {
        ESP_LOGI(TAG, "Button repress arrived during teardown; restarting chime");
        goto restart_playback;
    }

    return ESP_OK;
}

typedef enum {
    CHIME_CLAIM_HANDLED,
    CHIME_CLAIM_NORMAL,
    CHIME_CLAIM_CAMERA_ACK,
} chime_claim_t;

static chime_claim_t claim_chime_playback(bool interruptible,
                                          bool allow_camera_ack)
{
    enum {
        CLAIM_STARTED,
        CLAIM_CAMERA_ACK_STARTED,
        CLAIM_RETRIGGERED,
        CLAIM_RESTART_QUEUED,
        CLAIM_CAMERA_BLOCKED,
        CLAIM_STOPPING,
    } outcome;

    portENTER_CRITICAL(&s_claim_lock);
    if (atomic_load(&s_camera_overlap_mode)) {
        bool expected = false;
        if (!allow_camera_ack || !CONFIG_SMART_DOORBELL_CAMERA_OVERLAP_ACK) {
            outcome = CLAIM_CAMERA_BLOCKED;
        } else if (atomic_compare_exchange_strong(&s_is_playing, &expected, true)) {
            atomic_store(&s_retrigger_requested, false);
            atomic_store(&s_stop_requested, false);
            atomic_store(&s_interruptible, true);
            atomic_store(&s_low_power_overlap, true);
            outcome = CLAIM_CAMERA_ACK_STARTED;
        } else if (atomic_load(&s_stop_requested)) {
            outcome = CLAIM_STOPPING;
        } else if (atomic_load(&s_tearing_down)) {
            atomic_store(&s_restart_after_teardown, true);
            atomic_store(&s_interruptible, true);
            outcome = CLAIM_RESTART_QUEUED;
        } else {
            atomic_store(&s_retrigger_requested, true);
            atomic_store(&s_interruptible, true);
            outcome = CLAIM_RETRIGGERED;
        }
    } else {
        bool expected = false;
        if (atomic_compare_exchange_strong(&s_is_playing, &expected, true)) {
            atomic_store(&s_retrigger_requested, false);
            atomic_store(&s_stop_requested, false);
            atomic_store(&s_interruptible, interruptible);
            atomic_store(&s_low_power_overlap, false);
            outcome = CLAIM_STARTED;
        } else if (atomic_load(&s_stop_requested)) {
            outcome = CLAIM_STOPPING;
        } else if (atomic_load(&s_tearing_down)) {
            atomic_store(&s_restart_after_teardown, true);
            outcome = CLAIM_RESTART_QUEUED;
        } else {
            atomic_store(&s_retrigger_requested, true);
            if (interruptible) {
                atomic_store(&s_interruptible, true);
            }
            outcome = CLAIM_RETRIGGERED;
        }
    }
    portEXIT_CRITICAL(&s_claim_lock);

    if (outcome == CLAIM_CAMERA_BLOCKED) {
        ESP_LOGI(TAG, "Camera-overlap acknowledgement disabled; chime suppressed");
        return CHIME_CLAIM_HANDLED;
    }
    if (outcome == CLAIM_STOPPING) {
        ESP_LOGI(TAG, "Chime retrigger suppressed while playback is yielding");
        return CHIME_CLAIM_HANDLED;
    }
    if (outcome == CLAIM_RETRIGGERED) {
        // A physical repress must remain perceptible even while the local
        // speaker is already ringing. The owner task consumes this flag at
        // the next PCM chunk boundary and rewinds without releasing I2S.
        ESP_LOGI(TAG, "Chime already audible; queued immediate sample-0 retrigger");
        return CHIME_CLAIM_HANDLED;
    }
    if (outcome == CLAIM_RESTART_QUEUED) {
        ESP_LOGI(TAG, "Chime teardown active; queued immediate restart");
        return CHIME_CLAIM_HANDLED;
    }
    return outcome == CLAIM_CAMERA_ACK_STARTED
               ? CHIME_CLAIM_CAMERA_ACK
               : CHIME_CLAIM_NORMAL;
}

esp_err_t chime_player_play_sync(void)
{
    if (claim_chime_playback(false, false) != CHIME_CLAIM_NORMAL) {
        return ESP_OK;
    }
    return play_claimed_chime(portMAX_DELAY, g_doorbell_chime_pcm_len,
                              speaker_output_attenuation_db(),
                              speaker_output_ramp_ms(), true,
                              "local doorbell chime");
}

static void first_chime_play_task(void *pvParameters)
{
    (void)pvParameters;
    play_claimed_chime(portMAX_DELAY, g_doorbell_chime_pcm_len,
                       speaker_output_attenuation_db(),
                       speaker_output_ramp_ms(), true,
                       "local doorbell chime");
    vTaskDelete(NULL);
}

static void repress_chime_play_task(void *pvParameters)
{
    (void)pvParameters;
    (void)play_claimed_chime(0, g_doorbell_chime_pcm_len,
                             speaker_output_attenuation_db(),
                             speaker_output_ramp_ms(), true,
                             "local repress chime");
    vTaskDelete(NULL);
}

static void camera_overlap_ack_task(void *pvParameters)
{
    (void)pvParameters;
    const size_t playback_len =
        ((size_t)CHIME_SAMPLE_RATE_HZ *
         CONFIG_SMART_DOORBELL_CAMERA_OVERLAP_ACK_DURATION_MS / 1000U) * 4U;
    (void)play_claimed_chime(
        0, playback_len,
        CONFIG_SMART_DOORBELL_CAMERA_OVERLAP_ACK_ATTENUATION_DB,
        speaker_output_ramp_ms(), true,
        "quiet camera-overlap doorbell chime");
    vTaskDelete(NULL);
}

esp_err_t chime_player_play_async(void)
{
    if (claim_chime_playback(false, false) != CHIME_CLAIM_NORMAL) {
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(first_chime_play_task, "first_chime", 4096,
                                 NULL, configMAX_PRIORITIES - 2, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create first chime task");
        release_playback_claim();
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t chime_player_play_repress_async(void)
{
    const chime_claim_t claim = claim_chime_playback(true, true);
    if (claim == CHIME_CLAIM_HANDLED) {
        return ESP_OK;
    }

    const bool camera_ack = claim == CHIME_CLAIM_CAMERA_ACK;
    BaseType_t ret = xTaskCreate(
        camera_ack ? camera_overlap_ack_task : repress_chime_play_task,
        camera_ack ? "camera_ack" : "repress_chime", 4096,
        NULL, configMAX_PRIORITIES - 2, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create %s task",
                 camera_ack ? "camera acknowledgement" : "repress chime");
        release_playback_claim();
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

bool chime_player_is_playing(void)
{
    return atomic_load(&s_is_playing);
}

bool chime_player_is_interruptible(void)
{
    return atomic_load(&s_is_playing) && atomic_load(&s_interruptible);
}

bool chime_player_camera_overlap_active(void)
{
    return atomic_load(&s_is_playing) &&
           atomic_load(&s_low_power_overlap) &&
           atomic_load(&s_output_active);
}

void chime_player_set_camera_overlap_mode(bool enabled)
{
    portENTER_CRITICAL(&s_claim_lock);
    atomic_store(&s_camera_overlap_mode, enabled);
    portEXIT_CRITICAL(&s_claim_lock);
}

esp_err_t chime_player_wait_until_idle(TickType_t timeout_ticks)
{
    const TickType_t started = xTaskGetTickCount();
    while (atomic_load(&s_is_playing)) {
        if (timeout_ticks != portMAX_DELAY &&
            xTaskGetTickCount() - started >= timeout_ticks) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_OK;
}

void chime_player_request_stop(void)
{
    atomic_store(&s_stop_requested, true);
}
