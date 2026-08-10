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

#ifndef GATEWAY_TRIGGER_URL
#define GATEWAY_TRIGGER_URL GATEWAY_API_URL "/trigger"
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT    BIT1

#ifndef WIFI_MAX_RETRY
#define WIFI_MAX_RETRY 5
#endif

#ifndef SMART_DOORBELL_WIFI_BRINGUP_UPLOAD
#define SMART_DOORBELL_WIFI_BRINGUP_UPLOAD 0
#endif

#define WIFI_START_DELAY_SEC 10

static const char *TAG = "wifi_bringup";

#if SMART_DOORBELL_WIFI_BRINGUP_UPLOAD
static const char diagnostic_image[] =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"320\" height=\"240\" viewBox=\"0 0 320 240\">"
    "<rect width=\"320\" height=\"240\" fill=\"#101820\"/>"
    "<rect x=\"24\" y=\"24\" width=\"272\" height=\"192\" rx=\"8\" fill=\"#1f7a5c\"/>"
    "<text x=\"160\" y=\"112\" text-anchor=\"middle\" font-family=\"monospace\" font-size=\"22\" fill=\"#ffffff\">"
    "Wi-Fi Diagnostic</text>"
    "<text x=\"160\" y=\"144\" text-anchor=\"middle\" font-family=\"monospace\" font-size=\"14\" fill=\"#d9fff0\">"
    "ESP32-S3 upload test</text>"
    "</svg>";
#endif

typedef struct {
    EventGroupHandle_t event_group;
    esp_netif_t *netif;
    esp_event_handler_instance_t wifi_handler;
    esp_event_handler_instance_t ip_handler;
    int retry_count;
    bool netif_initialized;
    bool event_loop_created;
    bool wifi_initialized;
    bool wifi_started;
    bool stopping;
} wifi_bringup_state_t;

static wifi_bringup_state_t s_wifi_state;

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
    set_output_low(CAM_PWR_EN_PIN);
    gpio_set_pull_mode(CAM_PWR_EN_PIN, GPIO_PULLDOWN_ONLY);
    set_output_low(AMP_EN_PIN);
    set_output_low(I2S_AUDIO_WS);
    set_output_low(I2S_AUDIO_SCK);
    set_output_low(I2S_SPK_SD);
    gpio_reset_pin(I2S_MIC_SD);
    gpio_set_direction(I2S_MIC_SD, GPIO_MODE_INPUT);
    gpio_set_pull_mode(I2S_MIC_SD, GPIO_PULLDOWN_ONLY);
    set_output_low((gpio_num_t)CAM_PIN_XCLK);
    set_output_low((gpio_num_t)CAM_PIN_PWDN);
    set_output_low((gpio_num_t)CAM_PIN_RESET);
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
        if (state->stopping) {
            return;
        }
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
    memset(&s_wifi_state, 0, sizeof(s_wifi_state));
    s_wifi_state.retry_count = 0;
    s_wifi_state.event_group = xEventGroupCreate();
    if (!s_wifi_state.event_group) {
        ESP_LOGE(TAG, "failed to create Wi-Fi event group");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) {
        goto fail;
    }
    s_wifi_state.netif_initialized = true;

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        goto fail;
    }
    s_wifi_state.event_loop_created = true;

    s_wifi_state.netif = esp_netif_create_default_wifi_sta();
    if (!s_wifi_state.netif) {
        ESP_LOGE(TAG, "esp_netif_create_default_wifi_sta failed");
        err = ESP_FAIL;
        goto fail;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        goto fail;
    }
    s_wifi_state.wifi_initialized = true;

    err = esp_event_handler_instance_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &wifi_event_handler,
                                               &s_wifi_state,
                                               &s_wifi_state.wifi_handler);
    if (err != ESP_OK) {
        goto fail;
    }
    err = esp_event_handler_instance_register(IP_EVENT,
                                               IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler,
                                               &s_wifi_state,
                                               &s_wifi_state.ip_handler);
    if (err != ESP_OK) {
        goto fail;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
#ifdef WIFI_CHANNEL
            .channel = WIFI_CHANNEL,
#endif
        },
    };

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        goto fail;
    }
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        goto fail;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        goto fail;
    }
    s_wifi_state.wifi_started = true;

    // Disable modem sleep power save for minimum latency during active transfers
    (void)esp_wifi_set_ps(WIFI_PS_NONE);

#if defined(WIFI_STATIC_IP) && defined(WIFI_NETMASK) && defined(WIFI_GATEWAY)
    esp_netif_dhcpc_stop(s_wifi_state.netif);
    esp_netif_ip_info_t ip_info;
    memset(&ip_info, 0, sizeof(ip_info));
    ip_info.ip.addr = esp_ip4addr_aton(WIFI_STATIC_IP);
    ip_info.netmask.addr = esp_ip4addr_aton(WIFI_NETMASK);
    ip_info.gw.addr = esp_ip4addr_aton(WIFI_GATEWAY);
    esp_netif_set_ip_info(s_wifi_state.netif, &ip_info);
#if defined(WIFI_DNS)
    esp_netif_dns_info_t dns_info;
    dns_info.ip.u_addr.ip4.addr = esp_ip4addr_aton(WIFI_DNS);
    dns_info.ip.type = ESP_IPADDR_TYPE_V4;
    esp_netif_set_dns_info(s_wifi_state.netif, ESP_NETIF_DNS_MAIN, &dns_info);
#endif
    ESP_LOGI(TAG, "Configured static IP: %s", WIFI_STATIC_IP);
#endif

    ESP_LOGI(TAG, "Wi-Fi STA started; waiting for connection");
    return ESP_OK;

fail:
    ESP_LOGE(TAG, "Wi-Fi STA setup failed: %s", esp_err_to_name(err));
    (void)wifi_bringup_stop_bounded();
    return err;
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
    size_t offset = 0;
    while (offset < len) {
        int written = esp_http_client_write(client, data + offset, len - offset);
        if (written <= 0) {
            ESP_LOGE(TAG, "HTTP write failed for %s at %u/%u",
                     label, (unsigned)offset, (unsigned)len);
            return ESP_FAIL;
        }
        offset += (size_t)written;
    }

    return ESP_OK;
}

esp_err_t wifi_bringup_get_rssi(int *rssi_dbm)
{
    if (!rssi_dbm) {
        return ESP_ERR_INVALID_ARG;
    }
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err == ESP_OK) {
        *rssi_dbm = ap_info.rssi;
    }
    return err;
}

esp_err_t wifi_bringup_trigger_event_timeout(const char *event_id,
                                             const char *event_type,
                                             const char *device_id,
                                             const char *firmware_version,
                                             int timeout_ms)
{
    if (!event_id || strlen(event_id) < 8 || !event_type ||
        event_type[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    int rssi_dbm = 0;
    bool has_rssi = wifi_bringup_get_rssi(&rssi_dbm) == ESP_OK;
    char body[384];
    int body_len;
    if (has_rssi) {
        body_len = snprintf(body, sizeof(body),
                            "{\"eventId\":\"%s\",\"eventType\":\"%s\","
                            "\"deviceId\":\"%s\",\"firmwareVersion\":\"%s\","
                            "\"wifiRssiDbm\":%d}",
                            event_id, event_type,
                            device_id ? device_id : "",
                            firmware_version ? firmware_version : "",
                            rssi_dbm);
    } else {
        body_len = snprintf(body, sizeof(body),
                            "{\"eventId\":\"%s\",\"eventType\":\"%s\","
                            "\"deviceId\":\"%s\",\"firmwareVersion\":\"%s\"}",
                            event_id, event_type,
                            device_id ? device_id : "",
                            firmware_version ? firmware_version : "");
    }
    if (body_len < 0 || (size_t)body_len >= sizeof(body)) {
        return ESP_ERR_INVALID_SIZE;
    }

    esp_http_client_config_t config = {
        .url = GATEWAY_TRIGGER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = timeout_ms > 0 ? timeout_ms : 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
#ifdef GATEWAY_API_KEY
    esp_http_client_set_header(client, "X-API-Key", GATEWAY_API_KEY);
#endif
    esp_http_client_set_post_field(client, body, body_len);

    ESP_LOGI(TAG, "Sending early %s trigger id=%s (timeout=%d ms)", event_type, event_id, config.timeout_ms);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Early trigger HTTP status=%d", status_code);
        if (status_code < 200 || status_code >= 300) {
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGW(TAG, "Early trigger HTTP request failed: %s",
                 esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t wifi_bringup_trigger_event(const char *event_id,
                                     const char *event_type,
                                     const char *device_id,
                                     const char *firmware_version)
{
    return wifi_bringup_trigger_event_timeout(event_id, event_type, device_id,
                                               firmware_version, 5000);
}

esp_err_t wifi_bringup_close_session(const char *session_id, int timeout_ms)
{
    if (!session_id || strlen(session_id) < 8) return ESP_ERR_INVALID_ARG;
    char url[320];
    int length = snprintf(url, sizeof(url), "%s/sessions/%s/close",
                          GATEWAY_API_URL, session_id);
    if (length < 0 || (size_t)length >= sizeof(url)) return ESP_ERR_INVALID_SIZE;

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = timeout_ms > 0 ? timeout_ms : 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_ERR_NO_MEM;
#ifdef GATEWAY_API_KEY
    esp_http_client_set_header(client, "X-API-Key", GATEWAY_API_KEY);
#endif
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        const int status = esp_http_client_get_status_code(client);
        if (status < 200 || status >= 300) err = ESP_FAIL;
    }
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t wifi_bringup_complete_press(const char *press_id,
                                      uint32_t duration_ms,
                                      int timeout_ms)
{
    if (!press_id || strlen(press_id) < 8 || duration_ms > 15000) {
        return ESP_ERR_INVALID_ARG;
    }
    char url[320];
    int length = snprintf(url, sizeof(url), "%s/presses/%s/complete",
                          GATEWAY_API_URL, press_id);
    if (length < 0 || (size_t)length >= sizeof(url)) return ESP_ERR_INVALID_SIZE;
    char body[48];
    int body_length = snprintf(body, sizeof(body),
                               "{\"durationMs\":%u}",
                               (unsigned)duration_ms);
    if (body_length < 0 || (size_t)body_length >= sizeof(body)) {
        return ESP_ERR_INVALID_SIZE;
    }

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = timeout_ms > 0 ? timeout_ms : 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_ERR_NO_MEM;
    esp_http_client_set_header(client, "Content-Type", "application/json");
#ifdef GATEWAY_API_KEY
    esp_http_client_set_header(client, "X-API-Key", GATEWAY_API_KEY);
#endif
    esp_http_client_set_post_field(client, body, body_length);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        const int status = esp_http_client_get_status_code(client);
        if (status < 200 || status >= 300) err = ESP_FAIL;
    }
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t wifi_bringup_upload_jpeg_timeout(const uint8_t *jpeg,
                                           size_t jpeg_len,
                                           const char *event_type,
                                           const char *event_id,
                                           const char *device_id,
                                           const char *firmware_version,
                                           int timeout_ms)
{
    return wifi_bringup_upload_event_timeout(
        jpeg, jpeg_len, NULL, 0, event_type, event_id, device_id,
        firmware_version, timeout_ms);
}

esp_err_t wifi_bringup_upload_event_timeout(const uint8_t *jpeg,
                                            size_t jpeg_len,
                                            const uint8_t *audio,
                                            size_t audio_len,
                                            const char *event_type,
                                            const char *event_id,
                                            const char *device_id,
                                            const char *firmware_version,
                                            int timeout_ms)
{
    const bool has_image = jpeg && jpeg_len >= 4;
    const bool has_audio = audio && audio_len > 44;
    if ((!has_image && !has_audio) || !event_type || event_type[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    const char *boundary = "SmartDoorbellCaptureBoundary";
    char event_head[768];
    char image_head[192];
    char audio_head[192];
    const char *terminator = "\r\n--SmartDoorbellCaptureBoundary--\r\n";

    int rssi_dbm = 0;
    bool has_rssi = wifi_bringup_get_rssi(&rssi_dbm) == ESP_OK;
    char rssi_text[16] = "";
    if (has_rssi) {
        snprintf(rssi_text, sizeof(rssi_text), "%d", rssi_dbm);
    }
    int event_head_len = snprintf(
        event_head, sizeof(event_head),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"eventType\"\r\n\r\n"
        "%s\r\n"
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"eventId\"\r\n\r\n"
        "%s\r\n"
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"deviceId\"\r\n\r\n"
        "%s\r\n"
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"firmwareVersion\"\r\n\r\n"
        "%s\r\n"
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"wifiRssiDbm\"\r\n\r\n"
        "%s\r\n",
        boundary, event_type,
        boundary, event_id ? event_id : "",
        boundary, device_id ? device_id : "",
        boundary, firmware_version ? firmware_version : "",
        boundary, rssi_text);
    int image_head_len = snprintf(
        image_head, sizeof(image_head),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"image\"; filename=\"doorbell-qxga.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n\r\n",
        boundary);
    int audio_head_len = snprintf(
        audio_head, sizeof(audio_head),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"audio\"; filename=\"visitor-greeting.wav\"\r\n"
        "Content-Type: audio/wav\r\n\r\n",
        boundary);
    if (event_head_len < 0 || (size_t)event_head_len >= sizeof(event_head) ||
        image_head_len < 0 || (size_t)image_head_len >= sizeof(image_head) ||
        audio_head_len < 0 || (size_t)audio_head_len >= sizeof(audio_head)) {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t total_len = (size_t)event_head_len + strlen(terminator);
    if (has_image) {
        total_len += (size_t)image_head_len + jpeg_len;
    }
    if (has_audio) {
        total_len += (size_t)audio_head_len + audio_len;
        if (has_image) total_len += 2;
    }
    esp_http_client_config_t config = {
        .url = GATEWAY_API_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = timeout_ms > 0 ? timeout_ms : 30000,
    };

    ESP_LOGI(TAG, "Uploading event (%u-byte JPEG, %u-byte WAV, %u-byte body, timeout=%d ms)",
             (unsigned)jpeg_len, (unsigned)audio_len, (unsigned)total_len,
             config.timeout_ms);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_ERR_NO_MEM;
    }

    char content_type[96];
    snprintf(content_type, sizeof(content_type),
             "multipart/form-data; boundary=%s", boundary);
    esp_http_client_set_header(client, "Content-Type", content_type);
#ifdef GATEWAY_API_KEY
    esp_http_client_set_header(client, "X-API-Key", GATEWAY_API_KEY);
#endif

    esp_err_t err = esp_http_client_open(client, (int)total_len);
    if (err == ESP_OK) {
        err = write_http_part(client, "event head", event_head,
                              (size_t)event_head_len);
    }
    if (err == ESP_OK && has_image) {
        err = write_http_part(client, "image head", image_head,
                              (size_t)image_head_len);
    }
    if (err == ESP_OK && has_image) {
        err = write_http_part(client, "JPEG", (const char *)jpeg, jpeg_len);
    }
    if (err == ESP_OK && has_image && has_audio) {
        err = write_http_part(client, "media separator", "\r\n", 2);
    }
    if (err == ESP_OK && has_audio) {
        err = write_http_part(client, "audio head", audio_head,
                              (size_t)audio_head_len);
    }
    if (err == ESP_OK && has_audio) {
        err = write_http_part(client, "visitor WAV", (const char *)audio,
                              audio_len);
    }
    if (err == ESP_OK) {
        err = write_http_part(client, "terminator", terminator,
                              strlen(terminator));
    }

    int status_code = 0;
    if (err == ESP_OK) {
        (void)esp_http_client_fetch_headers(client);
        status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "QXGA upload HTTP status=%d", status_code);
        if (status_code < 200 || status_code >= 300) {
            err = ESP_FAIL;
        }
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return err;
}

#if SMART_DOORBELL_WIFI_BRINGUP_UPLOAD
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
#endif

esp_err_t wifi_bringup_connect_bounded_timeout(int timeout_ms)
{
    esp_err_t err = init_nvs_for_wifi();
    if (err != ESP_OK) {
        return err;
    }

    err = start_wifi_sta();
    if (err != ESP_OK) {
        return err;
    }

    int wait_ticks = timeout_ms > 0 ? pdMS_TO_TICKS(timeout_ms) : pdMS_TO_TICKS(20000);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_state.event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           wait_ticks);
    if (!(bits & WIFI_CONNECTED_BIT)) {
        err = bits & WIFI_FAILED_BIT ? ESP_FAIL : ESP_ERR_TIMEOUT;
        (void)wifi_bringup_stop_bounded();
        return err;
    }

    log_wifi_status();
    return ESP_OK;
}

esp_err_t wifi_bringup_connect_bounded(void)
{
    return wifi_bringup_connect_bounded_timeout(20000);
}

esp_err_t wifi_bringup_stop_bounded(void)
{
    esp_err_t first_err = ESP_OK;
    s_wifi_state.stopping = true;

    if (s_wifi_state.wifi_started) {
        esp_err_t err = esp_wifi_stop();
        if (first_err == ESP_OK && err != ESP_OK) {
            first_err = err;
        }
        s_wifi_state.wifi_started = false;
    }
    if (s_wifi_state.wifi_handler) {
        esp_err_t err = esp_event_handler_instance_unregister(
            WIFI_EVENT, ESP_EVENT_ANY_ID, s_wifi_state.wifi_handler);
        if (first_err == ESP_OK && err != ESP_OK) {
            first_err = err;
        }
        s_wifi_state.wifi_handler = NULL;
    }
    if (s_wifi_state.ip_handler) {
        esp_err_t err = esp_event_handler_instance_unregister(
            IP_EVENT, IP_EVENT_STA_GOT_IP, s_wifi_state.ip_handler);
        if (first_err == ESP_OK && err != ESP_OK) {
            first_err = err;
        }
        s_wifi_state.ip_handler = NULL;
    }
    if (s_wifi_state.wifi_initialized) {
        esp_err_t err = esp_wifi_deinit();
        if (first_err == ESP_OK && err != ESP_OK) {
            first_err = err;
        }
        s_wifi_state.wifi_initialized = false;
    }
    if (s_wifi_state.netif) {
        esp_netif_destroy_default_wifi(s_wifi_state.netif);
        s_wifi_state.netif = NULL;
    }
    if (s_wifi_state.event_loop_created) {
        esp_err_t err = esp_event_loop_delete_default();
        if (first_err == ESP_OK && err != ESP_OK) {
            first_err = err;
        }
        s_wifi_state.event_loop_created = false;
    }
    if (s_wifi_state.netif_initialized) {
        /*
         * ESP-IDF's lwIP-backed esp_netif_deinit() is deliberately not
         * implemented and always returns ESP_ERR_NOT_SUPPORTED once the
         * TCP/IP task exists. The stack is safe to initialize again, so keep
         * that process-global task and release the per-connection netif,
         * handlers, event loop, and Wi-Fi driver above.
         */
        s_wifi_state.netif_initialized = false;
    }
    if (s_wifi_state.event_group) {
        vEventGroupDelete(s_wifi_state.event_group);
        s_wifi_state.event_group = NULL;
    }
    s_wifi_state.stopping = false;

    ESP_LOGI(TAG,
             "Wi-Fi deinitialized; RF, netif, and event resources released");
    return first_err;
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
#if SMART_DOORBELL_WIFI_BRINGUP_UPLOAD
        esp_err_t upload_err = upload_diagnostic_event();
        if (upload_err == ESP_OK) {
            ESP_LOGI(TAG, "Diagnostic gateway upload succeeded");
        } else {
            ESP_LOGE(TAG, "Diagnostic gateway upload failed: %s", esp_err_to_name(upload_err));
        }
#else
        ESP_LOGI(TAG, "Diagnostic gateway upload is disabled");
#endif
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
