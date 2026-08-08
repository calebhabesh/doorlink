#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_bringup_connect_bounded(void);
esp_err_t wifi_bringup_connect_bounded_timeout(int timeout_ms);
esp_err_t wifi_bringup_trigger_event(const char *event_id,
                                     const char *event_type,
                                     const char *device_id,
                                     const char *firmware_version);
esp_err_t wifi_bringup_trigger_event_timeout(const char *event_id,
                                             const char *event_type,
                                             const char *device_id,
                                             const char *firmware_version,
                                             int timeout_ms);
esp_err_t wifi_bringup_upload_jpeg(const uint8_t *jpeg,
                                   size_t jpeg_len,
                                   const char *event_type,
                                   const char *event_id,
                                   const char *device_id,
                                   const char *firmware_version);
esp_err_t wifi_bringup_upload_jpeg_timeout(const uint8_t *jpeg,
                                           size_t jpeg_len,
                                           const char *event_type,
                                           const char *event_id,
                                           const char *device_id,
                                           const char *firmware_version,
                                           int timeout_ms);
esp_err_t wifi_bringup_get_rssi(int *rssi_dbm);
esp_err_t wifi_bringup_stop_bounded(void);
void run_wifi_bringup(void);

#ifdef __cplusplus
}
#endif
