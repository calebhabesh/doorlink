#include "chime_player.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "board_pins.h"
#include "doorbell_chime_pcm.h"
#include "ring_fade.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CHIME_SAMPLE_RATE_HZ 44100
#define CHIME_WRITE_CHUNK_BYTES 1024
#define CHIME_DEBOUNCE_TIME_US (15ULL * 1000ULL) // 15 ms debounce window

static const char *TAG = "chime_player";
static atomic_bool s_is_playing = false;
static atomic_bool s_retrigger_requested = false;

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

esp_err_t chime_player_play_sync(void)
{
    if (atomic_load(&s_is_playing)) {
        ESP_LOGI(TAG, "Chime already playing; retriggering from sample 0");
        atomic_store(&s_retrigger_requested, true);
        return ESP_OK;
    }
    atomic_store(&s_is_playing, true);
    atomic_store(&s_retrigger_requested, false);

    // Configure AMP_EN_PIN as output and enable MAX98357A amp
    gpio_reset_pin(AMP_EN_PIN);
    gpio_set_direction(AMP_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(AMP_EN_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10)); // Allow amp power-on settle

    i2s_chan_handle_t tx_chan = NULL;
    esp_err_t err = init_speaker_i2s(&tx_chan);
    if (err != ESP_OK) {
        gpio_set_level(AMP_EN_PIN, 0);
        atomic_store(&s_is_playing, false);
        return err;
    }

    ESP_LOGI(TAG, "Playing local doorbell chime (%u bytes PCM @ 44.1kHz stereo)...",
             (unsigned)g_doorbell_chime_pcm_len);

    size_t offset = 0;
    size_t bytes_written = 0;

    while (offset < g_doorbell_chime_pcm_len) {
        if (atomic_exchange(&s_retrigger_requested, false)) {
            offset = 0;
            ESP_LOGI(TAG, "⚡ BUTTON RETRIGGER DETECTED! Rewinding local chime to sample 0");
            ring_animation_start(150, 1000, 300);
        }

        size_t chunk_len = g_doorbell_chime_pcm_len - offset;
        if (chunk_len > CHIME_WRITE_CHUNK_BYTES) {
            chunk_len = CHIME_WRITE_CHUNK_BYTES;
        }

        err = i2s_channel_write(tx_chan, g_doorbell_chime_pcm + offset,
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

    // Mute amp first, then tear down I2S channel
    gpio_set_level(AMP_EN_PIN, 0);
    i2s_channel_disable(tx_chan);
    i2s_del_channel(tx_chan);

    ESP_LOGI(TAG, "Local doorbell chime playback completed cleanly");
    atomic_store(&s_is_playing, false);
    atomic_store(&s_retrigger_requested, false);
    return ESP_OK;
}

static void chime_play_task(void *pvParameters)
{
    (void)pvParameters;
    chime_player_play_sync();
    vTaskDelete(NULL);
}

esp_err_t chime_player_play_async(void)
{
    if (atomic_load(&s_is_playing)) {
        ESP_LOGI(TAG, "Async chime retrigger requested");
        atomic_store(&s_retrigger_requested, true);
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(chime_play_task, "chime_play_task", 4096, NULL,
                                 configMAX_PRIORITIES - 2, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create chime_play_task");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

bool chime_player_is_playing(void)
{
    return atomic_load(&s_is_playing);
}
