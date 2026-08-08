#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    size_t size;
    uint16_t width;
    uint16_t height;
} camera_owned_jpeg_t;

/*
 * Capture one production-candidate QXGA JPEG into owned PSRAM.
 *
 * On every return path the ESP camera frame is returned, the driver is
 * deinitialized, and GPIO42/U9 is disabled. The caller owns result->data only
 * after ESP_OK and must release it with camera_capture_release().
 */
esp_err_t camera_capture_qxga_owned(camera_owned_jpeg_t *result);
esp_err_t camera_capture_qxga_owned_timeout(camera_owned_jpeg_t *result, int timeout_ms);
void camera_capture_release(camera_owned_jpeg_t *result);

#ifdef __cplusplus
}
#endif
