#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "smart_doorbell";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Smart Doorbell System");

    // TODO: Initialize WiFi Station
    ESP_LOGI(TAG, "[Placeholder] WiFi Init");

    // TODO: Initialize Camera (OV5640)
    ESP_LOGI(TAG, "[Placeholder] Camera Init");

    // TODO: Initialize I2S (INMP441 & MAX98357A)
    ESP_LOGI(TAG, "[Placeholder] I2S Audio Init");

    // TODO: Setup Button Interrupts
    ESP_LOGI(TAG, "[Placeholder] Button Init");

    // TODO: Connect to MQTT Broker
    ESP_LOGI(TAG, "[Placeholder] MQTT Init");

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
