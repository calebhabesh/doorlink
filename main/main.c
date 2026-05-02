#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_camera.h"
#include "driver/i2s_std.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "config.h"
#include "esp_http_client.h"

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

// =======================
// I2S Audio Wiring
// =======================
// INMP441 Microphone (RX)
#define I2S_MIC_WS      18
#define I2S_MIC_SD      19 // DATA
#define I2S_MIC_SCK     20 // BCLK

// MAX98357A Speaker (TX)
#define I2S_SPK_WS      21 // LRC
#define I2S_SPK_SD      22 // DIN
#define I2S_SPK_SCK     41 // BCLK

static i2s_chan_handle_t rx_chan; // Microphone
static i2s_chan_handle_t tx_chan; // Speaker

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

static esp_err_t init_i2s(void)
{
    // 1. Allocate I2S channels
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan_cfg, &tx_chan, &rx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channels: %s", esp_err_to_name(err));
        return err;
    }

    // 2. Configure standard I2S for Speaker (TX)
    i2s_std_config_t tx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000), // 16 kHz sample rate
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_SPK_SCK,
            .ws   = I2S_SPK_WS,
            .dout = I2S_SPK_SD,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    err = i2s_channel_init_std_mode(tx_chan, &tx_std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S TX channel: %s", esp_err_to_name(err));
        return err;
    }

    // 3. Configure standard I2S for Microphone (RX)
    i2s_std_config_t rx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000), // 16 kHz sample rate
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_MIC_SCK,
            .ws   = I2S_MIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_MIC_SD,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    err = i2s_channel_init_std_mode(rx_chan, &rx_std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S RX channel: %s", esp_err_to_name(err));
        return err;
    }

    // 4. Enable both channels
    i2s_channel_enable(tx_chan);
    i2s_channel_enable(rx_chan);

    ESP_LOGI(TAG, "I2S Init Succeeded (TX/RX at 16kHz)");
    return ESP_OK;
}

#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void init_wifi(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished. Waiting for connection...");

    // Wait until connection is established
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
}

static void init_mqtt(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "MQTT Init Succeeded");
}

static esp_err_t upload_image_to_gateway(const uint8_t *image_data, size_t image_len)
{
    esp_err_t err = ESP_OK;
    
    esp_http_client_config_t config = {
        .url = GATEWAY_API_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    // Generate boundary
    const char *boundary = "----SmartDoorbellBoundary123456789";
    char content_type[64];
    snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
    esp_http_client_set_header(client, "Content-Type", content_type);

    // Build the multipart body
    const char *part1 = 
        "------SmartDoorbellBoundary123456789\r\n"
        "Content-Disposition: form-data; name=\"eventType\"\r\n\r\n"
        "DOORBELL_PRESS\r\n"
        "------SmartDoorbellBoundary123456789\r\n"
        "Content-Disposition: form-data; name=\"image\"; filename=\"dummy.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n\r\n";
    
    const char *part2 = "\r\n------SmartDoorbellBoundary123456789--\r\n";

    // Calculate total content length
    int total_len = strlen(part1) + image_len + strlen(part2);
    
    // Open connection
    err = esp_http_client_open(client, total_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    // Write parts
    esp_http_client_write(client, part1, strlen(part1));
    esp_http_client_write(client, (const char *)image_data, image_len);
    esp_http_client_write(client, part2, strlen(part2));

    // Fetch response
    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d", status_code, content_length);
    
    if (status_code == 200) {
        ESP_LOGI(TAG, "Image uploaded successfully!");
        char response_buf[256] = {0};
        int read_len = esp_http_client_read(client, response_buf, sizeof(response_buf) - 1);
        if (read_len > 0) {
            ESP_LOGI(TAG, "Gateway Response: %s", response_buf);
        }
    } else {
        ESP_LOGE(TAG, "Upload failed with status %d", status_code);
        err = ESP_FAIL;
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return err;
}

void app_main(void)
{
    // Initialize NVS (Required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initializing Smart Doorbell System");

    // 1. Determine wakeup cause
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();

    // For testing without the button, run on normal boot or EXT0
    if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT0 || wakeup_cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        ESP_LOGI(TAG, "Woke up from deep sleep (or normal boot). Starting network flow.");
        
        // 2. Initialize WiFi Station
        init_wifi();

        // 3. Dummy Image Generation & Upload
        ESP_LOGI(TAG, "Generating dummy image data...");
        const uint8_t dummy_image[] = "This is a fake JPEG payload to test the gateway.";
        size_t dummy_len = sizeof(dummy_image);
        
        ESP_LOGI(TAG, "Uploading dummy image...");
        upload_image_to_gateway(dummy_image, dummy_len);

        // 4. Initialize I2S (INMP441 & MAX98357A)
        if (init_i2s() == ESP_OK) {
            ESP_LOGI(TAG, "Audio peripherals ready for streaming");
        }

        // 5. Connect to MQTT Broker
        init_mqtt();

        // Simulating the time it takes to handle a doorbell event
        vTaskDelay(10000 / portTICK_PERIOD_MS);

    } else {
        ESP_LOGI(TAG, "Woke up from reset (cause: %d). Going to sleep immediately.", wakeup_cause);
    }

    ESP_LOGI(TAG, "Preparing to enter deep sleep...");

    // 4. Configure wakeup source
    // Enable pullup on the button pin so it defaults to HIGH
    rtc_gpio_pullup_en(DOORBELL_BUTTON_PIN);
    rtc_gpio_pulldown_dis(DOORBELL_BUTTON_PIN);
    
    // Configure EXT0 wakeup on LOW level (0) for the button press
    esp_sleep_enable_ext0_wakeup(DOORBELL_BUTTON_PIN, 0);

    ESP_LOGI(TAG, "Entering deep sleep now");
    
    // 5. Enter Deep Sleep
    esp_deep_sleep_start();
}
