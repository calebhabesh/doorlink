#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"
#include "mic_bringup.h"

#define MIC_SAMPLE_RATE_HZ 16000
#define MIC_READ_FRAMES    256
#define MIC_SLOT_COUNT     2

#ifndef SMART_DOORBELL_MIC_BRINGUP_ENABLE_I2S
#define SMART_DOORBELL_MIC_BRINGUP_ENABLE_I2S 1
#endif

static const char *TAG = "mic_bringup";

typedef struct {
    size_t count;
    int32_t min;
    int32_t max;
    uint64_t abs_sum;
    size_t zero_count;
    size_t stuck_count;
    int32_t previous;
    bool have_previous;
} mic_stats_t;

static void configure_safe_gpio_state(void)
{
    gpio_reset_pin(STATUS_LED_PIN);
    gpio_set_direction(STATUS_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(STATUS_LED_PIN, 0);

    gpio_reset_pin(BUTTON_LED_PIN);
    gpio_set_direction(BUTTON_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUTTON_LED_PIN, 0);

    gpio_reset_pin(AMP_EN_PIN);
    gpio_set_direction(AMP_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(AMP_EN_PIN, 0);

    gpio_reset_pin((gpio_num_t)CAM_PIN_XCLK);
    gpio_set_direction((gpio_num_t)CAM_PIN_XCLK, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)CAM_PIN_XCLK, 0);

    gpio_reset_pin((gpio_num_t)CAM_PIN_PWDN);
    gpio_set_direction((gpio_num_t)CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)CAM_PIN_PWDN, 0);

    gpio_reset_pin((gpio_num_t)CAM_PIN_RESET);
    gpio_set_direction((gpio_num_t)CAM_PIN_RESET, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)CAM_PIN_RESET, 0);
}

static void fault_blink_loop(void)
{
    bool led_on = false;

    while (true) {
        led_on = !led_on;
        gpio_set_level(STATUS_LED_PIN, led_on);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

static esp_err_t init_mic_i2s(i2s_chan_handle_t *rx_chan)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, rx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_SCK,
            .ws = I2S_AUDIO_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_MIC_SD,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

    err = i2s_channel_init_std_mode(*rx_chan, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2s_channel_enable(*rx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2S RX enabled: WS=GPIO%d SCK=GPIO%d SD=GPIO%d rate=%dHz 32-bit stereo",
             I2S_AUDIO_WS, I2S_AUDIO_SCK, I2S_MIC_SD, MIC_SAMPLE_RATE_HZ);
    return ESP_OK;
}

static int32_t raw_i2s_to_sample(int32_t raw)
{
    return raw >> 8;
}

static void stats_init(mic_stats_t *stats)
{
    *stats = (mic_stats_t) {
        .count = 0,
        .min = INT32_MAX,
        .max = INT32_MIN,
        .abs_sum = 0,
        .zero_count = 0,
        .stuck_count = 0,
        .previous = 0,
        .have_previous = false,
    };
}

static void stats_add_sample(mic_stats_t *stats, int32_t sample)
{
    if (sample < stats->min) {
        stats->min = sample;
    }
    if (sample > stats->max) {
        stats->max = sample;
    }

    int64_t sample64 = sample;
    stats->abs_sum += (uint64_t)(sample64 < 0 ? -sample64 : sample64);

    if (sample == 0) {
        stats->zero_count++;
    }
    if (stats->have_previous && sample == stats->previous) {
        stats->stuck_count++;
    }

    stats->previous = sample;
    stats->have_previous = true;
    stats->count++;
}

static const char *classify_stats(const mic_stats_t *stats)
{
    if (stats->count == 0) {
        return "NO_DATA";
    }
    if (stats->zero_count == stats->count) {
        return "ALL_ZERO";
    }
    if (stats->stuck_count + 1 >= stats->count) {
        return "STUCK";
    }
    if ((stats->max - stats->min) < 64) {
        return "LOW_VARIATION";
    }
    return "ACTIVE";
}

static void log_slot_stats(const char *slot_name, const mic_stats_t *stats)
{
    uint64_t avg_abs = stats->count ? stats->abs_sum / stats->count : 0;
    int32_t range = stats->count ? stats->max - stats->min : 0;

    ESP_LOGI(TAG,
             "%s status=%s count=%u min=%" PRId32 " max=%" PRId32
             " range=%" PRId32 " avg_abs=%" PRIu64 " zero=%u stuck=%u",
             slot_name,
             classify_stats(stats),
             (unsigned)stats->count,
             stats->count ? stats->min : 0,
             stats->count ? stats->max : 0,
             range,
             avg_abs,
             (unsigned)stats->zero_count,
             (unsigned)stats->stuck_count);
}

void run_mic_bringup(void)
{
    ESP_LOGI(TAG, "Starting mic-only bring-up firmware");
    ESP_LOGI(TAG, "Expected mic slot: LEFT, because MK1 LR pin is tied to GND");
    ESP_LOGI(TAG, "Camera, Wi-Fi, MQTT, speaker output, and deep sleep are disabled");

    configure_safe_gpio_state();

#if !SMART_DOORBELL_MIC_BRINGUP_ENABLE_I2S
    ESP_LOGI(TAG, "I2S is disabled for core USB/heartbeat isolation");
    bool core_led_on = false;
    unsigned heartbeat = 0;

    while (true) {
        core_led_on = !core_led_on;
        gpio_set_level(STATUS_LED_PIN, core_led_on);
        ESP_LOGI(TAG, "core heartbeat %u", heartbeat++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#endif

    i2s_chan_handle_t rx_chan = NULL;
    if (init_mic_i2s(&rx_chan) != ESP_OK) {
        ESP_LOGE(TAG, "I2S init failed; blinking status LED quickly");
        fault_blink_loop();
    }

    bool led_on = false;
    int32_t samples[MIC_READ_FRAMES * MIC_SLOT_COUNT];

    while (true) {
        size_t bytes_read = 0;
        esp_err_t err = i2s_channel_read(rx_chan, samples, sizeof(samples), &bytes_read, pdMS_TO_TICKS(1000));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2s_channel_read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        mic_stats_t left;
        mic_stats_t right;
        stats_init(&left);
        stats_init(&right);

        size_t slots_read = bytes_read / sizeof(samples[0]);
        for (size_t i = 0; i + 1 < slots_read; i += MIC_SLOT_COUNT) {
            stats_add_sample(&left, raw_i2s_to_sample(samples[i]));
            stats_add_sample(&right, raw_i2s_to_sample(samples[i + 1]));
        }

        ESP_LOGI(TAG, "read bytes=%u frames=%u", (unsigned)bytes_read, (unsigned)(slots_read / MIC_SLOT_COUNT));
        log_slot_stats("left", &left);
        log_slot_stats("right", &right);

        led_on = !led_on;
        gpio_set_level(STATUS_LED_PIN, led_on);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
