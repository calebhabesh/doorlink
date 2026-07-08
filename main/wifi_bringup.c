#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "board_pins.h"
#include "config.h"
#include "wifi_bringup.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT    BIT1

#ifndef WIFI_MAX_RETRY
#define WIFI_MAX_RETRY 5
#endif

#define WIFI_START_DELAY_SEC 10

static const char *TAG = "wifi_bringup";

static const char diagnostic_image[] =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"320\" height=\"240\" viewBox=\"0 0 320 240\">"
    "<rect width=\"320\" height=\"240\" fill=\"#101820\"/>"
    "<rect x=\"24\" y=\"24\" width=\"272\" height=\"192\" rx=\"8\" fill=\"#1f7a5c\"/>"
    "<text x=\"160\" y=\"112\" text-anchor=\"middle\" font-family=\"monospace\" font-size=\"22\" fill=\"#ffffff\">"
    "Wi-Fi Diagnostic</text>"
    "<text x=\"160\" y=\"144\" text-anchor=\"middle\" font-family=\"monospace\" font-size=\"14\" fill=\"#d9fff0\">"
    "ESP32-S3 upload test</text>"
    "</svg>";

typedef struct {
    EventGroupHandle_t event_group;
    int retry_count;
} wifi_bringup_state_t;

static wifi_bringup_state_t s_wifi_state;

static void configure_safe_gpio_state(void)
{
    gpio_reset_pin(STATUS_LED_PIN);
    gpio_set_direction(STATUS_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(STATUS_LED_PIN, 0);

    gpio_reset_pin(BUTTON_LED_PIN);
    gpio_set_direction(BUTTON_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUTTON_LED_PIN, 0);

    gpio_reset_pin(AMP_EN_PIN);
    gpio_set_direction(AMP_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(AMP_EN_PIN, 0);
}

static void fault_blink_loop(void)
{
    bool led_on = false;

    while (true) {
        led_on = !led_on;
        gpio_set_level(STATUS_LED_PIN, led_on);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_bringup_state_t *state = (wifi_bringup_state_t *)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (state->retry_count < WIFI_MAX_RETRY) {
            state->retry_count++;
            ESP_LOGW(TAG, "Wi-Fi disconnected; retrying (%d/%d)", state->retry_count, WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "Wi-Fi failed after %d retries", WIFI_MAX_RETRY);
            xEventGroupSetBits(state->event_group, WIFI_FAILED_BIT);
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        state->retry_count = 0;
        ESP_LOGI(TAG, "Wi-Fi connected: ip=" IPSTR " gateway=" IPSTR " netmask=" IPSTR,
                 IP2STR(&event->ip_info.ip),
                 IP2STR(&event->ip_info.gw),
                 IP2STR(&event->ip_info.netmask));
        xEventGroupSetBits(state->event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t init_nvs_for_wifi(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "failed to erase NVS");
        err = nvs_flash_init();
    }

    return err;
}

static esp_err_t start_wifi_sta(void)
{
    s_wifi_state.retry_count = 0;
    s_wifi_state.event_group = xEventGroupCreate();
    if (!s_wifi_state.event_group) {
        ESP_LOGE(TAG, "failed to create Wi-Fi event group");
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "esp_event_loop_create_default failed");

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    if (!netif) {
        ESP_LOGE(TAG, "esp_netif_create_default_wifi_sta failed");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "esp_wifi_init failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            &s_wifi_state,
                                                            NULL),
                        TAG, "register WIFI_EVENT handler failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            &s_wifi_state,
                                                            NULL),
                        TAG, "register IP_EVENT handler failed");

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "esp_wifi_set_mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "esp_wifi_set_config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start failed");

    ESP_LOGI(TAG, "Wi-Fi STA started; waiting for connection");
    return ESP_OK;
}

static void log_wifi_status(void)
{
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi status: rssi=%d dBm channel=%u authmode=%d free_heap=%" PRIu32,
                 ap_info.rssi,
                 ap_info.primary,
                 ap_info.authmode,
                 esp_get_free_heap_size());
    } else {
        ESP_LOGW(TAG, "esp_wifi_sta_get_ap_info failed: %s", esp_err_to_name(err));
    }
}

static esp_err_t write_http_part(esp_http_client_handle_t client, const char *label, const char *data, size_t len)
{
    int written = esp_http_client_write(client, data, len);
    if (written < 0) {
        ESP_LOGE(TAG, "HTTP write failed for %s", label);
        return ESP_FAIL;
    }
    if ((size_t)written != len) {
        ESP_LOGE(TAG, "HTTP short write for %s: %d/%u", label, written, (unsigned)len);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t upload_diagnostic_event(void)
{
    const char *boundary = "SmartDoorbellDiagnosticBoundary";
    const char *event_head =
        "--SmartDoorbellDiagnosticBoundary\r\n"
        "Content-Disposition: form-data; name=\"eventType\"\r\n\r\n"
        "DIAGNOSTIC_UPLOAD\r\n";
    const char *image_head =
        "--SmartDoorbellDiagnosticBoundary\r\n"
        "Content-Disposition: form-data; name=\"image\"; filename=\"wifi-diagnostic.svg\"\r\n"
        "Content-Type: image/svg+xml\r\n\r\n";
    const char *terminator = "\r\n--SmartDoorbellDiagnosticBoundary--\r\n";

    size_t image_len = strlen(diagnostic_image);
    size_t total_len = strlen(event_head) + strlen(image_head) + image_len + strlen(terminator);

    esp_http_client_config_t config = {
        .url = GATEWAY_API_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,
    };

    ESP_LOGI(TAG, "Uploading diagnostic event to %s (%u bytes)", GATEWAY_API_URL, (unsigned)total_len);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        return ESP_FAIL;
    }

    char content_type[96];
    snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
    esp_http_client_set_header(client, "Content-Type", content_type);
#ifdef GATEWAY_API_KEY
    esp_http_client_set_header(client, "X-API-Key", GATEWAY_API_KEY);
#endif

    esp_err_t err = esp_http_client_open(client, total_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    err = write_http_part(client, "event head", event_head, strlen(event_head));
    if (err == ESP_OK) {
        err = write_http_part(client, "image head", image_head, strlen(image_head));
    }
    if (err == ESP_OK) {
        err = write_http_part(client, "diagnostic image", diagnostic_image, image_len);
    }
    if (err == ESP_OK) {
        err = write_http_part(client, "terminator", terminator, strlen(terminator));
    }
    if (err != ESP_OK) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return err;
    }

    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Diagnostic upload HTTP status=%d content_length=%d", status_code, content_length);

    char response[160];
    int response_len = esp_http_client_read_response(client, response, sizeof(response) - 1);
    if (response_len >= 0) {
        response[response_len] = '\0';
        ESP_LOGI(TAG, "Diagnostic upload response: %s", response);
    } else {
        ESP_LOGW(TAG, "Diagnostic upload response read failed: %d", response_len);
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return status_code >= 200 && status_code < 300 ? ESP_OK : ESP_FAIL;
}

void run_wifi_bringup(void)
{
    ESP_LOGI(TAG, "Starting Wi-Fi-only bring-up firmware");
    ESP_LOGI(TAG, "Camera, mic, speaker, MQTT, upload, and deep sleep are disabled");

    configure_safe_gpio_state();

    esp_err_t err = init_nvs_for_wifi();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
        fault_blink_loop();
    }

    ESP_LOGI(TAG, "RF is still off. Starting Wi-Fi in %d seconds.", WIFI_START_DELAY_SEC);
    for (int seconds_left = WIFI_START_DELAY_SEC; seconds_left > 0; seconds_left--) {
        gpio_set_level(STATUS_LED_PIN, seconds_left % 2);
        ESP_LOGI(TAG, "pre-Wi-Fi countdown: %d", seconds_left);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    gpio_set_level(STATUS_LED_PIN, 0);

    err = start_wifi_sta();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi start failed: %s", esp_err_to_name(err));
        fault_blink_loop();
    }

    EventBits_t bits = xEventGroupWaitBits(s_wifi_state.event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(20000));

    if (bits & WIFI_CONNECTED_BIT) {
        log_wifi_status();
        esp_err_t upload_err = upload_diagnostic_event();
        if (upload_err == ESP_OK) {
            ESP_LOGI(TAG, "Diagnostic gateway upload succeeded");
        } else {
            ESP_LOGE(TAG, "Diagnostic gateway upload failed: %s", esp_err_to_name(upload_err));
        }
    } else if (bits & WIFI_FAILED_BIT) {
        ESP_LOGE(TAG, "Wi-Fi bring-up failed");
        fault_blink_loop();
    } else {
        ESP_LOGE(TAG, "Wi-Fi connection timed out");
        fault_blink_loop();
    }

    bool led_on = false;
    uint32_t heartbeat = 0;
    while (true) {
        led_on = !led_on;
        gpio_set_level(STATUS_LED_PIN, led_on);

        if ((heartbeat % 5) == 0) {
            log_wifi_status();
        } else {
            ESP_LOGI(TAG, "Wi-Fi heartbeat %" PRIu32, heartbeat);
        }

        heartbeat++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
