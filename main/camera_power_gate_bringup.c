#include "camera_power_gate_bringup.h"

#include <stdint.h>

#include "board_pins.h"
#include "camera_power.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef CAMERA_GATE_PREPARE_DELAY_MS
#define CAMERA_GATE_PREPARE_DELAY_MS 3000
#endif

#ifndef CAMERA_GATE_PULSE_MS
#define CAMERA_GATE_PULSE_MS 100
#endif

#ifndef CAMERA_GATE_CAMERA_CONNECTED
#define CAMERA_GATE_CAMERA_CONNECTED 0
#endif

static const char *TAG = "camera_power_gate";

static void set_output_low(gpio_num_t pin)
{
    gpio_reset_pin(pin);
    gpio_set_level(pin, 0);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
}

void run_camera_power_gate_bringup(void)
{
    set_output_low(STATUS_LED_PIN);
    set_output_low(BUTTON_LED_PIN);
    set_output_low(AMP_EN_PIN);

    esp_err_t err = camera_power_prepare();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera safe-state preparation failed: %s",
                 esp_err_to_name(err));
        camera_power_disable();
    } else {
        ESP_LOGW(TAG, "REV C CAMERA-RAILS-ONLY TEST: camera %s",
                 CAMERA_GATE_CAMERA_CONNECTED ? "connected in safe standby" : "must be disconnected");
        ESP_LOGI(TAG, "Amplifier, I2S, Wi-Fi, MQTT, camera driver, and sleep are disabled");
        ESP_LOGI(TAG, "U9 is off; one %d ms pulse starts in %d ms",
                 CAMERA_GATE_PULSE_MS, CAMERA_GATE_PREPARE_DELAY_MS);
        vTaskDelay(pdMS_TO_TICKS(CAMERA_GATE_PREPARE_DELAY_MS));

        gpio_set_level(STATUS_LED_PIN, 1);
        err = camera_power_enable();
        if (err == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(CAMERA_GATE_PULSE_MS));
        } else {
            ESP_LOGE(TAG, "Camera rail enable failed: %s", esp_err_to_name(err));
        }

        esp_err_t disable_err = camera_power_disable();
        gpio_set_level(STATUS_LED_PIN, 0);
        if (disable_err != ESP_OK) {
            ESP_LOGE(TAG, "Camera rail shutdown failed: %s",
                     esp_err_to_name(disable_err));
        } else if (err == ESP_OK) {
            ESP_LOGI(TAG, "One-shot U9 pulse complete; camera rails are off permanently");
        }
    }

    for (uint32_t heartbeat = 0;; ++heartbeat) {
        ESP_LOGI(TAG, "post-test heartbeat %u; U9 off", (unsigned)heartbeat);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
