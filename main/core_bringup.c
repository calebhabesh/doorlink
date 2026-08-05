#include "core_bringup.h"

#include <inttypes.h>
#include <stdint.h>

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "core_bringup";

static const gpio_num_t s_camera_input_pins[] = {
    CAM_PIN_D0,
    CAM_PIN_D1,
    CAM_PIN_D2,
    CAM_PIN_D3,
    CAM_PIN_D4,
    CAM_PIN_D5,
    CAM_PIN_D6,
    CAM_PIN_D7,
    CAM_PIN_PCLK,
    CAM_PIN_HREF,
    CAM_PIN_SIOD,
    CAM_PIN_SIOC,
    CAM_PIN_VSYNC,
};

static void set_output_low(gpio_num_t pin)
{
    ESP_ERROR_CHECK(gpio_reset_pin(pin));
    ESP_ERROR_CHECK(gpio_set_level(pin, 0));
    ESP_ERROR_CHECK(gpio_set_direction(pin, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(pin, 0));
}

void core_bringup_configure_safe_gpio_state(bool camera_attached)
{
    /*
     * Hold all currently unused outputs low. XCLK, PWDN, and RESET reach the
     * 2.8 V camera domain through resistor networks. With no camera, keeping
     * all three low prevents an uninitialized 3.3 V GPIO from lifting the
     * unloaded rail. With a camera attached, PWDN must instead be high so the
     * sensor remains in its lowest-load hardware power-down state.
     */
    set_output_low(CAM_PWR_EN_PIN);
    ESP_ERROR_CHECK(gpio_set_pull_mode(CAM_PWR_EN_PIN, GPIO_PULLDOWN_ONLY));
    set_output_low(AMP_EN_PIN);
    set_output_low(I2S_AUDIO_WS);
    set_output_low(I2S_AUDIO_SCK);
    set_output_low(I2S_SPK_SD);
    ESP_ERROR_CHECK(gpio_reset_pin(I2S_MIC_SD));
    ESP_ERROR_CHECK(gpio_set_direction(I2S_MIC_SD, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(I2S_MIC_SD, GPIO_PULLDOWN_ONLY));
    set_output_low(STATUS_LED_PIN);
    set_output_low(BUTTON_LED_PIN);
    set_output_low((gpio_num_t)CAM_PIN_XCLK);
    set_output_low((gpio_num_t)CAM_PIN_RESET);
    set_output_low((gpio_num_t)CAM_PIN_PWDN);
    if (camera_attached) {
        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)CAM_PIN_PWDN, 1));
    }

    for (size_t i = 0; i < sizeof(s_camera_input_pins) / sizeof(s_camera_input_pins[0]); ++i) {
        gpio_num_t pin = s_camera_input_pins[i];
        ESP_ERROR_CHECK(gpio_reset_pin(pin));
        ESP_ERROR_CHECK(gpio_set_direction(pin, GPIO_MODE_INPUT));
        ESP_ERROR_CHECK(gpio_set_pull_mode(pin, GPIO_FLOATING));
    }
}

static void log_memory_configuration(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;

    esp_chip_info(&chip_info);
    esp_err_t flash_err = esp_flash_get_size(NULL, &flash_size);

    ESP_LOGI(TAG, "Target: %s, revision v%d.%d, %d CPU core(s)",
             CONFIG_IDF_TARGET,
             chip_info.revision / 100,
             chip_info.revision % 100,
             chip_info.cores);
    if (flash_err == ESP_OK) {
        ESP_LOGI(TAG, "Flash detected: %" PRIu32 " bytes (%" PRIu32 " MiB); image mode DIO",
                 flash_size, flash_size / (1024U * 1024U));
    } else {
        ESP_LOGE(TAG, "Flash size query failed: %s", esp_err_to_name(flash_err));
    }

    ESP_LOGI(TAG, "PSRAM initialized=%s size=%" PRIu32 " bytes (%" PRIu32 " MiB) heap=%u",
             esp_psram_is_initialized() ? "yes" : "no",
             (uint32_t)esp_psram_get_size(),
             (uint32_t)(esp_psram_get_size() / (1024U * 1024U)),
             (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM));

    if (flash_err != ESP_OK || flash_size != 16U * 1024U * 1024U) {
        ESP_LOGE(TAG, "N16R8 CHECK FAIL: expected 16 MiB flash");
    } else if (!esp_psram_is_initialized() || esp_psram_get_size() != 8U * 1024U * 1024U) {
        ESP_LOGE(TAG, "N16R8 CHECK FAIL: expected initialized 8 MiB PSRAM");
    } else {
        ESP_LOGI(TAG, "N16R8 CHECK PASS: 16 MiB flash and 8 MiB octal PSRAM");
    }
}

void run_core_bringup(bool camera_attached)
{
    core_bringup_configure_safe_gpio_state(camera_attached);

    /*
     * Native USB disconnects briefly across reset. Keep the safe GPIO state
     * active immediately, but allow the host monitor time to reconnect before
     * emitting the one-time bring-up evidence.
     */
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Smart Doorbell core-only bring-up image");
    ESP_LOGI(TAG, "Camera, I2S, amplifier, Wi-Fi, MQTT, and deep sleep are disabled");
    ESP_LOGI(TAG, "I2S standby: WS=GPIO%d LOW, SCK=GPIO%d LOW, speaker data=GPIO%d LOW, mic data=GPIO%d input pulldown",
             I2S_AUDIO_WS, I2S_AUDIO_SCK, I2S_SPK_SD, I2S_MIC_SD);
    ESP_LOGI(TAG, "Camera-attached idle=%s", camera_attached ? "yes" : "no");
    ESP_LOGI(TAG, "Camera controls: PWR_EN=GPIO%d LOW, XCLK=GPIO%d LOW, PWDN=GPIO%d %s, RESET=GPIO%d LOW",
             CAM_PWR_EN_PIN, CAM_PIN_XCLK, CAM_PIN_PWDN,
             camera_attached ? "HIGH" : "LOW", CAM_PIN_RESET);
    log_memory_configuration();

    for (uint32_t heartbeat = 0;; ++heartbeat) {
        ESP_LOGI(TAG, "heartbeat %" PRIu32 "; safe GPIO state active", heartbeat);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
