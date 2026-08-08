#include "ring_fade.h"

#include <stdbool.h>

#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
static esp_timer_handle_t s_breathing_timer;
static bool s_breathing_active;
static bool s_breathing_dir_down;

#define BREATHING_FADE_MS  1400
#define BREATHING_TIMER_MS 1500
#define BREATHING_LOW_DUTY (RING_LEDC_MAX_DUTY / 4U) // 25% duty floor

static void breathing_timer_cb(void *unused)
{
    (void)unused;
    if (!s_channel_configured || !s_breathing_active) {
        return;
    }

    // While physical button is held down, hold 100% brightness
    if (gpio_get_level(DOORBELL_BUTTON_PIN) == 0) {
        (void)ledc_fade_stop(RING_LEDC_MODE, RING_LEDC_CHANNEL);
        (void)ledc_set_duty(RING_LEDC_MODE, RING_LEDC_CHANNEL, RING_LEDC_MAX_DUTY);
        (void)ledc_update_duty(RING_LEDC_MODE, RING_LEDC_CHANNEL);
        s_breathing_dir_down = true;
        if (s_breathing_timer && s_breathing_active) {
            esp_timer_start_once(s_breathing_timer, 200000ULL);
        }
        return;
    }

    uint32_t target_duty;
    if (s_breathing_dir_down) {
        target_duty = BREATHING_LOW_DUTY;
        s_breathing_dir_down = false;
    } else {
        target_duty = RING_LEDC_MAX_DUTY;
        s_breathing_dir_down = true;
    }

    (void)ledc_fade_stop(RING_LEDC_MODE, RING_LEDC_CHANNEL);
    esp_err_t err = ledc_set_fade_with_time(
        RING_LEDC_MODE, RING_LEDC_CHANNEL, target_duty, BREATHING_FADE_MS);
    if (err == ESP_OK) {
        (void)ledc_fade_start(RING_LEDC_MODE, RING_LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
    }

    if (s_breathing_timer && s_breathing_active) {
        esp_timer_start_once(s_breathing_timer, BREATHING_TIMER_MS * 1000ULL);
    }
}

esp_err_t ring_animation_start(uint32_t fade_in_ms, uint32_t hold_ms,
                               uint32_t fade_out_ms)
{
    (void)hold_ms;
    (void)fade_out_ms;
    if (fade_in_ms == 0) {
        fade_in_ms = 200;
    }

    // Stop existing breathing timer if active
    if (s_breathing_timer) {
        (void)esp_timer_stop(s_breathing_timer);
    }
    s_breathing_active = false;

    if (!s_channel_configured) {
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
    }

    if (!s_fade_installed) {
        esp_err_t err = ledc_fade_func_install(0);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ring_fade_stop();
            return err;
        }
        s_fade_installed = true;
    }

    // Stop any in-progress fade before flashing back up to 100% duty
    if (s_channel_configured) {
        (void)ledc_fade_stop(RING_LEDC_MODE, RING_LEDC_CHANNEL);
    }

    // Start initial rapid fade in to 100% duty
    esp_err_t err = ledc_set_fade_with_time(RING_LEDC_MODE, RING_LEDC_CHANNEL,
                                           RING_LEDC_MAX_DUTY, fade_in_ms);
    if (err == ESP_OK) {
        err = ledc_fade_start(RING_LEDC_MODE, RING_LEDC_CHANNEL,
                              LEDC_FADE_NO_WAIT);
    }
    if (err != ESP_OK) {
        ring_fade_stop();
        return err;
    }

    s_breathing_active = true;
    s_breathing_dir_down = true;

    if (!s_breathing_timer) {
        const esp_timer_create_args_t timer_args = {
            .callback = breathing_timer_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "ring-breathing",
            .skip_unhandled_events = true,
        };
        err = esp_timer_create(&timer_args, &s_breathing_timer);
        if (err != ESP_OK) {
            ring_fade_stop();
            return err;
        }
    }

    // Start continuous breathing pulse after initial fade in
    err = esp_timer_start_once(s_breathing_timer, (uint64_t)fade_in_ms * 1000ULL);
    if (err != ESP_OK) {
        ring_fade_stop();
        return err;
    }

    ESP_LOGI(TAG, "GPIO%d Ring Doorbell continuous breathing animation started",
             BUTTON_LED_PIN);
    return ESP_OK;
}

void ring_fade_stop(void)
{
    s_breathing_active = false;
    if (s_breathing_timer) {
        (void)esp_timer_stop(s_breathing_timer);
        (void)esp_timer_delete(s_breathing_timer);
        s_breathing_timer = NULL;
    }

    if (s_channel_configured && s_fade_installed) {
        // Smooth 400ms fade down to 0 before shutting off LEDC channel
        esp_err_t err = ledc_set_fade_with_time(RING_LEDC_MODE, RING_LEDC_CHANNEL, 0, 400);
        if (err == ESP_OK) {
            (void)ledc_fade_start(RING_LEDC_MODE, RING_LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
            vTaskDelay(pdMS_TO_TICKS(400));
        }
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
