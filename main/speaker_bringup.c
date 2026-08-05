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
#include "speaker_bringup.h"

#define SPEAKER_SAMPLE_RATE_HZ 16000
#define SPEAKER_FRAMES         256
#define SPEAKER_TONE_CHUNKS    20

static const char *TAG = "speaker_bringup";

static void set_output_low(gpio_num_t pin)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
}

static void configure_safe_gpio_state(void)
{
    set_output_low(STATUS_LED_PIN);
    set_output_low(BUTTON_LED_PIN);
    set_output_low(CAM_PWR_EN_PIN);
    gpio_set_pull_mode(CAM_PWR_EN_PIN, GPIO_PULLDOWN_ONLY);
    set_output_low(AMP_EN_PIN);
    set_output_low((gpio_num_t)CAM_PIN_XCLK);
    set_output_low((gpio_num_t)CAM_PIN_PWDN);
    set_output_low((gpio_num_t)CAM_PIN_RESET);
}

static void fault_blink_loop(void)
{
    while (true) {
        gpio_set_level(STATUS_LED_PIN, !gpio_get_level(STATUS_LED_PIN));
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

static esp_err_t init_speaker_i2s(i2s_chan_handle_t *tx_chan)
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    esp_err_t err = i2s_new_channel(&chan_cfg, tx_chan, NULL);
    if (err != ESP_OK) {
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SPEAKER_SAMPLE_RATE_HZ),
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
        return err;
    }
    return i2s_channel_enable(*tx_chan);
}

static void fill_quiet_tone(int16_t *samples)
{
    static const int16_t triangle[16] = {
        0, 128, 256, 384, 512, 384, 256, 128,
        0, -128, -256, -384, -512, -384, -256, -128,
    };

    for (size_t frame = 0; frame < SPEAKER_FRAMES; ++frame) {
        int16_t sample = triangle[frame % 16];
        samples[frame * 2] = sample;
        samples[frame * 2 + 1] = sample;
    }
}

void run_speaker_bringup(void)
{
    configure_safe_gpio_state();
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGW(TAG, "SPEAKER-ONLY BRING-UP: camera, mic RX, Wi-Fi, MQTT, and sleep disabled");
    ESP_LOGW(TAG, "J6 is a bridged output; neither speaker terminal may be grounded");
    ESP_LOGI(TAG, "Tone: 1 kHz triangle, amplitude 512/32767, about 320 ms every 2 s");

    i2s_chan_handle_t tx_chan = NULL;
    esp_err_t err = init_speaker_i2s(&tx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S TX initialization failed: %s", esp_err_to_name(err));
        fault_blink_loop();
    }

    int16_t tone[SPEAKER_FRAMES * 2];
    int16_t silence[SPEAKER_FRAMES * 2] = {0};
    fill_quiet_tone(tone);

    size_t bytes_written = 0;
    i2s_channel_write(tx_chan, silence, sizeof(silence), &bytes_written,
                      pdMS_TO_TICKS(100));

    while (true) {
        gpio_set_level(AMP_EN_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(STATUS_LED_PIN, 1);

        for (int chunk = 0; chunk < SPEAKER_TONE_CHUNKS; ++chunk) {
            err = i2s_channel_write(tx_chan, tone, sizeof(tone), &bytes_written,
                                    pdMS_TO_TICKS(100));
            if (err != ESP_OK || bytes_written != sizeof(tone)) {
                ESP_LOGE(TAG, "I2S write failed: %s, bytes=%u",
                         esp_err_to_name(err), (unsigned)bytes_written);
                gpio_set_level(AMP_EN_PIN, 0);
                fault_blink_loop();
            }
        }

        i2s_channel_write(tx_chan, silence, sizeof(silence), &bytes_written,
                          pdMS_TO_TICKS(100));
        gpio_set_level(AMP_EN_PIN, 0);
        gpio_set_level(STATUS_LED_PIN, 0);
        ESP_LOGI(TAG, "Low-level tone burst complete");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
