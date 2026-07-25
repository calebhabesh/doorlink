#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"
#include "button_bringup.h"

static const char *TAG = "button_bringup";

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
    set_output_low(AMP_EN_PIN);
    set_output_low((gpio_num_t)CAM_PIN_XCLK);
    set_output_low((gpio_num_t)CAM_PIN_PWDN);
    set_output_low((gpio_num_t)CAM_PIN_RESET);

    gpio_reset_pin(DOORBELL_BUTTON_PIN);
    gpio_set_direction(DOORBELL_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DOORBELL_BUTTON_PIN, GPIO_FLOATING);
}

void run_button_bringup(void)
{
    configure_safe_gpio_state();
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGW(TAG, "BUTTON-ONLY BRING-UP: camera, I2S, amplifier, Wi-Fi, MQTT, and sleep disabled");
    ESP_LOGI(TAG, "J8: pin1 signal, pin2 GND, pin3 LED+ via R9, pin4 LED GND");
    ESP_LOGI(TAG, "Button LED will toggle once per second; press state is active LOW");

    int stable_level = gpio_get_level(DOORBELL_BUTTON_PIN);
    int candidate_level = stable_level;
    int candidate_count = 0;
    bool led_on = false;
    int led_ticks = 0;

    ESP_LOGI(TAG, "Initial button state: %s",
             stable_level == 0 ? "PRESSED (LOW)" : "RELEASED (HIGH)");

    while (true) {
        int level = gpio_get_level(DOORBELL_BUTTON_PIN);
        if (level == candidate_level) {
            candidate_count++;
        } else {
            candidate_level = level;
            candidate_count = 1;
        }

        if (candidate_count >= 3 && candidate_level != stable_level) {
            stable_level = candidate_level;
            ESP_LOGI(TAG, "Button state: %s",
                     stable_level == 0 ? "PRESSED (LOW)" : "RELEASED (HIGH)");
            gpio_set_level(STATUS_LED_PIN, stable_level == 0);
        }

        if (++led_ticks >= 100) {
            led_ticks = 0;
            led_on = !led_on;
            gpio_set_level(BUTTON_LED_PIN, led_on);
            ESP_LOGI(TAG, "Button LED: %s", led_on ? "ON" : "OFF");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
