#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_camera.h"

static const char *TAG = "smart_doorbell";

// Assuming GPIO 4 is used for the doorbell button, connected to GND when pressed
#define DOORBELL_BUTTON_PIN GPIO_NUM_4

// =======================
// Camera Wiring (OV5640)
// =======================
// Adjust these pins to match your ESP32-S3 wiring diagram
#define CAM_PIN_PWDN    -1 // power down is not used
#define CAM_PIN_RESET   -1 // software reset will be performed
#define CAM_PIN_XCLK    10
#define CAM_PIN_SIOD    40
#define CAM_PIN_SIOC    39
#define CAM_PIN_D7      17
#define CAM_PIN_D6      16
#define CAM_PIN_D5      15
#define CAM_PIN_D4      14
#define CAM_PIN_D3      13
#define CAM_PIN_D2      12
#define CAM_PIN_D1      11
#define CAM_PIN_D0      9
#define CAM_PIN_VSYNC   6
#define CAM_PIN_HREF    7
#define CAM_PIN_PCLK    8

static esp_err_t init_camera(void)
{
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

        // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
        .frame_size = FRAMESIZE_UXGA,   // QQVGA-UXGA. For OV5640 you can even go higher.

        .jpeg_quality = 12, // 0-63, lower number means higher quality
        .fb_count = 2,      // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    // Initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    ESP_LOGI(TAG, "Camera Init Succeeded");
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Smart Doorbell System");

    // 1. Determine wakeup cause
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();

    if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT0) {
        ESP_LOGI(TAG, "Woke up from deep sleep due to doorbell button press!");
        
        // TODO: Initialize WiFi Station
        ESP_LOGI(TAG, "[Placeholder] WiFi Init");

        // 2. Initialize Camera (OV5640)
        if (init_camera() == ESP_OK) {
            // Take a picture
            camera_fb_t *pic = esp_camera_fb_get();
            if (!pic) {
                ESP_LOGE(TAG, "Failed to capture image");
            } else {
                ESP_LOGI(TAG, "Picture taken! Size: %zu bytes", pic->len);
                // Return the frame buffer back to the driver for reuse
                esp_camera_fb_return(pic);
            }
        }

        // TODO: Initialize I2S (INMP441 & MAX98357A)
        ESP_LOGI(TAG, "[Placeholder] I2S Audio Init");

        // TODO: Connect to MQTT Broker
        ESP_LOGI(TAG, "[Placeholder] MQTT Init");

        // Simulating the time it takes to handle a doorbell event
        vTaskDelay(10000 / portTICK_PERIOD_MS);

    } else {
        ESP_LOGI(TAG, "Woke up from normal boot or reset (cause: %d). Going to sleep immediately.", wakeup_cause);
    }

    ESP_LOGI(TAG, "Preparing to enter deep sleep...");

    // 3. Configure wakeup source
    // Enable pullup on the button pin so it defaults to HIGH
    rtc_gpio_pullup_en(DOORBELL_BUTTON_PIN);
    rtc_gpio_pulldown_dis(DOORBELL_BUTTON_PIN);
    
    // Configure EXT0 wakeup on LOW level (0) for the button press
    esp_sleep_enable_ext0_wakeup(DOORBELL_BUTTON_PIN, 0);

    ESP_LOGI(TAG, "Entering deep sleep now");
    
    // 4. Enter Deep Sleep
    esp_deep_sleep_start();
}
