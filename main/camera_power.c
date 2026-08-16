#include "camera_power.h"

#include <stddef.h>
#include <stdint.h>

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "camera_power";

/*
 * TPS22919 typically turns on in under 2 ms at 3.6 V. The longer delay also
 * allows the downstream 2.8 V and 1.5 V LDO outputs to settle before the
 * camera driver releases RESET/PWDN or starts XCLK.
 */
#define CAMERA_RAIL_SETTLE_MS 20
#define CAMERA_OFF_SETTLE_MS  5

static const gpio_num_t s_camera_input_pins[] = {
    (gpio_num_t)CAM_PIN_D0,
    (gpio_num_t)CAM_PIN_D1,
    (gpio_num_t)CAM_PIN_D2,
    (gpio_num_t)CAM_PIN_D3,
    (gpio_num_t)CAM_PIN_D4,
    (gpio_num_t)CAM_PIN_D5,
    (gpio_num_t)CAM_PIN_D6,
    (gpio_num_t)CAM_PIN_D7,
    (gpio_num_t)CAM_PIN_PCLK,
    (gpio_num_t)CAM_PIN_HREF,
    (gpio_num_t)CAM_PIN_SIOD,
    (gpio_num_t)CAM_PIN_SIOC,
    (gpio_num_t)CAM_PIN_VSYNC,
};

static esp_err_t set_output_level(gpio_num_t pin, uint32_t level)
{
    esp_err_t err = gpio_reset_pin(pin);
    if (err != ESP_OK) {
        return err;
    }

    /*
     * Load the output latch before enabling the output driver, then write it
     * once more afterward. This avoids an active-high pulse on U9 ON.
     */
    err = gpio_set_level(pin, level);
    if (err != ESP_OK) {
        return err;
    }
    err = gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    if (err != ESP_OK) {
        return err;
    }
    return gpio_set_level(pin, level);
}

static esp_err_t float_camera_inputs(void)
{
    for (size_t i = 0; i < sizeof(s_camera_input_pins) / sizeof(s_camera_input_pins[0]); ++i) {
        gpio_num_t pin = s_camera_input_pins[i];
        esp_err_t err = gpio_reset_pin(pin);
        if (err != ESP_OK) {
            return err;
        }
        err = gpio_set_direction(pin, GPIO_MODE_INPUT);
        if (err != ESP_OK) {
            return err;
        }
        err = gpio_set_pull_mode(pin, GPIO_FLOATING);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t camera_power_prepare(void)
{
    esp_err_t err = set_output_level(CAM_PWR_EN_PIN, 0);
    if (err != ESP_OK) {
        return err;
    }
    err = gpio_set_pull_mode(CAM_PWR_EN_PIN, GPIO_PULLDOWN_ONLY);
    if (err != ESP_OK) {
        return err;
    }

    err = set_output_level((gpio_num_t)CAM_PIN_XCLK, 0);
    if (err != ESP_OK) {
        return err;
    }
    err = set_output_level((gpio_num_t)CAM_PIN_RESET, 0);
    if (err != ESP_OK) {
        return err;
    }
    err = set_output_level((gpio_num_t)CAM_PIN_PWDN, 1);
    if (err != ESP_OK) {
        return err;
    }

    return float_camera_inputs();
}

esp_err_t camera_power_enable(void)
{
    esp_err_t err = camera_power_prepare();
    if (err != ESP_OK) {
        return err;
    }

    /*
     * Last-line power-safety defense: controller arbitration should already have
     * drained and muted U4, but never energize the camera domain if any future
     * caller reaches this driver while the speaker amplifier is still on.
     */
    if (gpio_get_level(AMP_EN_PIN) != 0) {
        ESP_LOGE(TAG,
                 "Refusing camera power-up while speaker amplifier GPIO%d is enabled",
                 AMP_EN_PIN);
        return ESP_ERR_INVALID_STATE;
    }

    err = gpio_set_level(CAM_PWR_EN_PIN, 1);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(CAMERA_RAIL_SETTLE_MS));
    ESP_LOGI(TAG, "Rev C camera rails enabled by GPIO%d", CAM_PWR_EN_PIN);
    return ESP_OK;
}

esp_err_t camera_power_disable(void)
{
    esp_err_t first_err = ESP_OK;
    esp_err_t err;

    err = set_output_level((gpio_num_t)CAM_PIN_XCLK, 0);
    if (first_err == ESP_OK && err != ESP_OK) {
        first_err = err;
    }
    err = set_output_level((gpio_num_t)CAM_PIN_RESET, 0);
    if (first_err == ESP_OK && err != ESP_OK) {
        first_err = err;
    }
    err = set_output_level((gpio_num_t)CAM_PIN_PWDN, 1);
    if (first_err == ESP_OK && err != ESP_OK) {
        first_err = err;
    }
    err = float_camera_inputs();
    if (first_err == ESP_OK && err != ESP_OK) {
        first_err = err;
    }

    err = set_output_level(CAM_PWR_EN_PIN, 0);
    if (first_err == ESP_OK && err != ESP_OK) {
        first_err = err;
    }
    err = gpio_set_pull_mode(CAM_PWR_EN_PIN, GPIO_PULLDOWN_ONLY);
    if (first_err == ESP_OK && err != ESP_OK) {
        first_err = err;
    }

    vTaskDelay(pdMS_TO_TICKS(CAMERA_OFF_SETTLE_MS));
    err = set_output_level((gpio_num_t)CAM_PIN_PWDN, 0);
    if (first_err == ESP_OK && err != ESP_OK) {
        first_err = err;
    }
    if (first_err == ESP_OK) {
        ESP_LOGI(TAG, "Rev C camera rails disabled by GPIO%d", CAM_PWR_EN_PIN);
    }
    return first_err;
}
