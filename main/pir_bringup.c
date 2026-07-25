#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"
#include "pir_bringup.h"

static const char *TAG = "pir_bringup";

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

    gpio_reset_pin(PIR_WAKE_PIN);
    gpio_set_direction(PIR_WAKE_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIR_WAKE_PIN, GPIO_FLOATING);
}

void run_pir_bringup(void)
{
    configure_safe_gpio_state();
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGW(TAG, "PIR-ONLY BRING-UP: camera, I2S, amplifier, Wi-Fi, MQTT, and sleep disabled");
    ESP_LOGI(TAG, "J7: pin1 3V3, pin2 PIR_SENSOR_OUT, pin3 GND");
    ESP_LOGI(TAG, "Default path: J7.2 through R30 to GPIO3; R29 is the pulldown");
    ESP_LOGW(TAG, "Allow the AM312 to stabilize before judging motion transitions");

    int stable_level = gpio_get_level(PIR_WAKE_PIN);
    int candidate_level = stable_level;
    int candidate_count = 0;
    int report_ticks = 0;

    gpio_set_level(STATUS_LED_PIN, stable_level);
    ESP_LOGI(TAG, "Initial PIR state: %s",
             stable_level ? "MOTION/HIGH" : "IDLE/LOW");

    while (true) {
        int level = gpio_get_level(PIR_WAKE_PIN);
        if (level == candidate_level) {
            candidate_count++;
        } else {
            candidate_level = level;
            candidate_count = 1;
        }

        if (candidate_count >= 5 && candidate_level != stable_level) {
            stable_level = candidate_level;
            gpio_set_level(STATUS_LED_PIN, stable_level);
            ESP_LOGI(TAG, "PIR state: %s",
                     stable_level ? "MOTION/HIGH" : "IDLE/LOW");
        }

        if (++report_ticks >= 500) {
            report_ticks = 0;
            ESP_LOGI(TAG, "PIR heartbeat: %s",
                     stable_level ? "MOTION/HIGH" : "IDLE/LOW");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
