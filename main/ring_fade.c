#include "ring_fade.h"

#include <stdbool.h>

#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

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
static esp_timer_handle_t s_fade_out_timer;
static uint32_t s_fade_out_ms;

static void start_fade_out(void *unused)
{
    (void)unused;
    if (!s_channel_configured) {
        return;
    }

    esp_err_t err = ledc_set_fade_with_time(
        RING_LEDC_MODE, RING_LEDC_CHANNEL, 0, s_fade_out_ms);
    if (err == ESP_OK) {
        err = ledc_fade_start(RING_LEDC_MODE, RING_LEDC_CHANNEL,
                              LEDC_FADE_NO_WAIT);
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "GPIO%d ring fade-out started over %u ms",
                 BUTTON_LED_PIN, (unsigned)s_fade_out_ms);
    } else {
        ESP_LOGE(TAG, "GPIO%d ring fade-out failed: %s", BUTTON_LED_PIN,
                 esp_err_to_name(err));
    }
}

esp_err_t ring_animation_start(uint32_t fade_in_ms, uint32_t hold_ms,
                               uint32_t fade_out_ms)
{
    if (fade_in_ms == 0 || fade_out_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ring_fade_stop();

    /* D3 provides the immediate RTC-stub acknowledgement. Start the button
     * ring dark so its only visible transition is a smooth fade in. */
    gpio_set_level(BUTTON_LED_PIN, 0);

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
        .duty = 0,
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

    err = ledc_set_fade_with_time(RING_LEDC_MODE, RING_LEDC_CHANNEL,
                                  RING_LEDC_MAX_DUTY, fade_in_ms);
    if (err == ESP_OK) {
        err = ledc_fade_start(RING_LEDC_MODE, RING_LEDC_CHANNEL,
                              LEDC_FADE_NO_WAIT);
    }
    if (err != ESP_OK) {
        ring_fade_stop();
        return err;
    }

    s_fade_out_ms = fade_out_ms;
    const esp_timer_create_args_t timer_args = {
        .callback = start_fade_out,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ring-fade-out",
        .skip_unhandled_events = true,
    };
    err = esp_timer_create(&timer_args, &s_fade_out_timer);
    if (err == ESP_OK) {
        const uint64_t delay_us =
            ((uint64_t)fade_in_ms + (uint64_t)hold_ms) * 1000ULL;
        err = esp_timer_start_once(s_fade_out_timer, delay_us);
    }
    if (err != ESP_OK) {
        ring_fade_stop();
        return err;
    }

    ESP_LOGI(TAG,
             "GPIO%d ring animation: fade in %u ms, hold %u ms, fade out %u ms",
             BUTTON_LED_PIN, (unsigned)fade_in_ms, (unsigned)hold_ms,
             (unsigned)fade_out_ms);
    return ESP_OK;
}

void ring_fade_stop(void)
{
    if (s_fade_out_timer) {
        (void)esp_timer_stop(s_fade_out_timer);
        (void)esp_timer_delete(s_fade_out_timer);
        s_fade_out_timer = NULL;
    }
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
