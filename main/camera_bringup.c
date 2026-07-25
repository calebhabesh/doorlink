#include "camera_bringup.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board_pins.h"
#include "camera_power.h"
#include "driver/gpio.h"
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "camera_bringup";

static void configure_safe_outputs(void)
{
    gpio_reset_pin(AMP_EN_PIN);
    gpio_set_direction(AMP_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(AMP_EN_PIN, 0);

    gpio_reset_pin(BUTTON_LED_PIN);
    gpio_set_direction(BUTTON_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUTTON_LED_PIN, 0);

    gpio_reset_pin(STATUS_LED_PIN);
    gpio_set_direction(STATUS_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(STATUS_LED_PIN, 0);
}

static bool jpeg_has_markers(const camera_fb_t *fb)
{
    if (!fb || !fb->buf || fb->len < 4) {
        return false;
    }

    const uint8_t *buf = fb->buf;
    return buf[0] == 0xff && buf[1] == 0xd8 &&
           buf[fb->len - 2] == 0xff && buf[fb->len - 1] == 0xd9;
}

void run_camera_bringup(void)
{
    configure_safe_outputs();

    ESP_LOGI(TAG, "Camera bring-up starting; Wi-Fi, mic, and amp are disabled");
    ESP_LOGI(TAG, "PSRAM initialized=%s size=%u heap_total=%u",
             esp_psram_is_initialized() ? "yes" : "no",
             (unsigned)esp_psram_get_size(),
             (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "Pins: PWR_EN=%d XCLK=%d PCLK=%d VSYNC=%d HREF=%d SDA=%d SCL=%d RST=%d PWDN=%d",
             CAM_PWR_EN_PIN,
             CAM_PIN_XCLK, CAM_PIN_PCLK, CAM_PIN_VSYNC, CAM_PIN_HREF,
             CAM_PIN_SIOD, CAM_PIN_SIOC, CAM_PIN_RESET, CAM_PIN_PWDN);

    camera_config_t camera_config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        .xclk_freq_hz = 10000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_QQVGA,
        .jpeg_quality = 20,
        .fb_count = 1,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = camera_power_enable();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera rail enable failed: %s", esp_err_to_name(err));
        esp_err_t disable_err = camera_power_disable();
        if (disable_err != ESP_OK) {
            ESP_LOGE(TAG, "Camera safe shutdown also failed: %s",
                     esp_err_to_name(disable_err));
        }
    } else {
        err = esp_camera_init(&camera_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(err));
        } else {
            sensor_t *sensor = esp_camera_sensor_get();
            if (sensor) {
                ESP_LOGI(TAG, "Sensor detected: PID=0x%04x MID=0x%02x%02x addr=0x%02x",
                         sensor->id.PID, sensor->id.MIDH, sensor->id.MIDL, sensor->slv_addr);
            }

            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                ESP_LOGE(TAG, "Frame capture failed");
            } else {
                ESP_LOGI(TAG, "Frame captured: %ux%u len=%u format=%d jpeg_markers=%s",
                         (unsigned)fb->width, (unsigned)fb->height, (unsigned)fb->len,
                         fb->format, jpeg_has_markers(fb) ? "valid" : "invalid");
                esp_camera_fb_return(fb);
            }

            ESP_LOGI(TAG, "Stopping camera driver after one-frame test");
            esp_camera_deinit();
        }

        esp_err_t disable_err = camera_power_disable();
        if (disable_err != ESP_OK) {
            ESP_LOGE(TAG, "Camera rail shutdown failed: %s",
                     esp_err_to_name(disable_err));
        }
    }

    for (uint32_t i = 0;; ++i) {
        gpio_set_level(STATUS_LED_PIN, i % 2);
        ESP_LOGI(TAG, "Camera bring-up heartbeat %u", (unsigned)i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
