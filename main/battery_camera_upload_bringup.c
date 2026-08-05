#include "battery_camera_upload_bringup.h"

#include <stdbool.h>
#include <stdint.h>

#include "board_pins.h"
#include "camera_capture.h"
#include "camera_power.h"
#include "core_bringup.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_bringup.h"

static const char *TAG = "battery_camera_upload";

#define START_DELAY_SECONDS 60

static void hold_result(bool passed)
{
    gpio_set_level(STATUS_LED_PIN, passed ? 1 : 0);
    for (uint32_t heartbeat = 0;; ++heartbeat) {
        if (!passed) {
            gpio_set_level(STATUS_LED_PIN, heartbeat % 2U);
            vTaskDelay(pdMS_TO_TICKS(150));
        } else {
            ESP_LOGI(TAG, "PASS hold: camera off, RF off, owned JPEG released");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

void run_battery_camera_upload_bringup(void)
{
    camera_owned_jpeg_t jpeg = {0};
    bool wifi_started = false;

    core_bringup_configure_safe_gpio_state(true);
    ESP_LOGW(TAG, "REV C BATTERY-ONLY QXGA CAPTURE/UPLOAD TEST");
    ESP_LOGI(TAG, "Audio, amplifier, MQTT, and sleep are disabled");
    ESP_LOGI(TAG, "Starting once in %d seconds", START_DELAY_SECONDS);

    for (int remaining = START_DELAY_SECONDS; remaining > 0; --remaining) {
        gpio_set_level(STATUS_LED_PIN, remaining % 2);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    gpio_set_level(STATUS_LED_PIN, 0);

    esp_err_t err = camera_capture_qxga_owned(&jpeg);
    if (err == ESP_OK) {
        err = wifi_bringup_connect_bounded();
        wifi_started = err == ESP_OK;
    }
    if (err == ESP_OK) {
        err = wifi_bringup_upload_jpeg(
            jpeg.data, jpeg.size, "BATTERY_QXGA_BRINGUP",
            NULL, NULL, NULL);
    }

    camera_capture_release(&jpeg);
    if (wifi_started) {
        esp_err_t stop_err = wifi_bringup_stop_bounded();
        if (err == ESP_OK && stop_err != ESP_OK) {
            err = stop_err;
        }
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "BATTERY QXGA CAPTURE/UPLOAD PASS");
        hold_result(true);
    }

    ESP_LOGE(TAG, "BATTERY QXGA CAPTURE/UPLOAD FAIL: %s", esp_err_to_name(err));
    (void)camera_power_disable();
    hold_result(false);
}
