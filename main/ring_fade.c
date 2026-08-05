#include "ring_fade.h"

#include <stdbool.h>

#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "ring_fade";

/* Camera XCLK uses timer/channel 0. Keep the diagnostic ring fade isolated
 * on timer/channel 1 so the eventual production design cannot retune XCLK. */
#define RING_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define RING_LEDC_TIMER      LEDC_TIMER_1
#define RING_LEDC_CHANNEL    LEDC_CHANNEL_1
#define RING_LEDC_RESOLUTION LEDC_TIMER_10_BIT
#define RING_LEDC_MAX_DUTY   ((1U << 10U) - 1U)
#define RING_LEDC_FREQUENCY  5000

static bool s_fade_installed;
static bool s_channel_configured;

esp_err_t ring_fade_start(uint32_t fade_ms)
{
    if (fade_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ring_fade_stop();

    /* The wake stub and GPIO acknowledgement already made the ring fully
     * bright. Configure PWM at the same perceived level before fading. */
    gpio_set_level(BUTTON_LED_PIN, 1);

    const ledc_timer_config_t timer = {
        .speed_mode = RING_LEDC_MODE,
        .duty_resolution = RING_LEDC_RESOLUTION,
        .timer_num = RING_LEDC_TIMER,
        .freq_hz = RING_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };
    esp_err_t err = ledc_timer_config(&timer);
    if (err != ESP_OK) {
        return err;
    }

    const ledc_channel_config_t channel = {
        .gpio_num = BUTTON_LED_PIN,
        .speed_mode = RING_LEDC_MODE,
        .channel = RING_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = RING_LEDC_TIMER,
        .duty = RING_LEDC_MAX_DUTY,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {
            .output_invert = 0,
        },
    };
    err = ledc_channel_config(&channel);
    if (err != ESP_OK) {
        return err;
    }
    s_channel_configured = true;

    err = ledc_fade_func_install(0);
    if (err != ESP_OK) {
        ring_fade_stop();
        return err;
    }
    s_fade_installed = true;

    err = ledc_set_fade_with_time(RING_LEDC_MODE, RING_LEDC_CHANNEL, 0,
                                  fade_ms);
    if (err == ESP_OK) {
        err = ledc_fade_start(RING_LEDC_MODE, RING_LEDC_CHANNEL,
                              LEDC_FADE_NO_WAIT);
    }
    if (err != ESP_OK) {
        ring_fade_stop();
        return err;
    }

    ESP_LOGI(TAG, "GPIO%d ring fade started: full to off over %u ms",
             BUTTON_LED_PIN, (unsigned)fade_ms);
    return ESP_OK;
}

void ring_fade_stop(void)
{
    if (s_channel_configured) {
        (void)ledc_stop(RING_LEDC_MODE, RING_LEDC_CHANNEL, 0);
        s_channel_configured = false;
    }
    if (s_fade_installed) {
        ledc_fade_func_uninstall();
        s_fade_installed = false;
    }
    gpio_reset_pin(BUTTON_LED_PIN);
    gpio_set_direction(BUTTON_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUTTON_LED_PIN, 0);
}
