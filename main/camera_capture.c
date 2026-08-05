#include "camera_capture.h"

#include <stdbool.h>
#include <string.h>

#include "board_pins.h"
#include "camera_power.h"
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ov5640_mode_fix.h"

static const char *TAG = "camera_capture";

#define CAMERA_XCLK_HZ 10000000
#define CAMERA_MAINS_HZ 60
#define CAMERA_GAIN_CEILING 0x0020
#define CAMERA_POST_BLC_CONVERGENCE_FRAMES 8

static bool jpeg_has_markers(const camera_fb_t *frame)
{
    return frame && frame->buf && frame->len >= 4 &&
           frame->buf[0] == 0xff && frame->buf[1] == 0xd8 &&
           frame->buf[frame->len - 2] == 0xff &&
           frame->buf[frame->len - 1] == 0xd9;
}

static esp_err_t discard_frames(unsigned count, const char *reason)
{
    for (unsigned i = 0; i < count; ++i) {
        camera_fb_t *frame = esp_camera_fb_get();
        if (!frame) {
            ESP_LOGE(TAG, "%s frame %u/%u failed", reason, i + 1U, count);
            return ESP_FAIL;
        }
        esp_camera_fb_return(frame);
    }

    ESP_LOGI(TAG, "Discarded %u %s frame(s)", count, reason);
    return ESP_OK;
}

void camera_capture_release(camera_owned_jpeg_t *result)
{
    if (!result) {
        return;
    }
    if (result->data) {
        heap_caps_free(result->data);
    }
    memset(result, 0, sizeof(*result));
}

esp_err_t camera_capture_qxga_owned(camera_owned_jpeg_t *result)
{
    if (!result) {
        return ESP_ERR_INVALID_ARG;
    }
    camera_capture_release(result);

    bool camera_initialized = false;
    camera_fb_t *frame = NULL;
    int64_t capture_started_us = esp_timer_get_time();
    esp_err_t err = camera_power_enable();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera rail enable failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    const camera_config_t config = {
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
        .xclk_freq_hz = CAMERA_XCLK_HZ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_QXGA,
        .jpeg_quality = 12,
        .fb_count = 1,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera initialization failed: %s", esp_err_to_name(err));
        goto cleanup;
    }
    camera_initialized = true;
    ESP_LOGI(TAG, "Camera timing init=%.1f ms",
             (double)(esp_timer_get_time() - capture_started_us) / 1000.0);

    sensor_t *sensor = esp_camera_sensor_get();
    if (!sensor || sensor->id.PID != OV5640_PID) {
        ESP_LOGE(TAG, "Expected OV5640 sensor is unavailable");
        err = ESP_ERR_NOT_SUPPORTED;
        goto cleanup;
    }

    int sensor_err = 0;
    sensor_err |= sensor->set_vflip(sensor, 1);
    sensor_err |= sensor->set_hmirror(sensor, 1);
    sensor_err |= sensor->set_gain_ctrl(sensor, 1);
    sensor_err |= sensor->set_exposure_ctrl(sensor, 1);
    sensor_err |= sensor->set_whitebal(sensor, 1);
    sensor_err |= sensor->set_awb_gain(sensor, 1);
    sensor_err |= sensor->set_wb_mode(sensor, 0);
    if (sensor_err != 0) {
        ESP_LOGE(TAG, "OV5640 automatic/orientation setup failed: %d", sensor_err);
        err = ESP_FAIL;
        goto cleanup;
    }

    err = ov5640_apply_full_readout_fix(
        sensor, FRAMESIZE_QXGA, OV5640_FULL_READOUT_BLC_FRAMES, false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OV5640 full-readout setup failed: %s",
                 esp_err_to_name(err));
        goto cleanup;
    }

    ov5640_aec_timing_t timing = {0};
    err = ov5640_configure_aec_timing(
        sensor, CAMERA_XCLK_HZ, CAMERA_MAINS_HZ, 0, &timing);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OV5640 AEC timing setup failed: %s",
                 esp_err_to_name(err));
        goto cleanup;
    }

    err = ov5640_set_auto_gain_ceiling(sensor, CAMERA_GAIN_CEILING);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OV5640 gain ceiling setup failed: %s",
                 esp_err_to_name(err));
        goto cleanup;
    }

    int64_t stage_started_us = esp_timer_get_time();
    err = discard_frames(OV5640_FULL_READOUT_BLC_FRAMES, "BLC settling");
    if (err != ESP_OK) {
        goto cleanup;
    }
    ESP_LOGI(TAG, "Camera timing BLC=%0.1f ms total=%0.1f ms",
             (double)(esp_timer_get_time() - stage_started_us) / 1000.0,
             (double)(esp_timer_get_time() - capture_started_us) / 1000.0);
    err = ov5640_finish_blc_recalibration(sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OV5640 BLC finish failed: %s", esp_err_to_name(err));
        goto cleanup;
    }
    stage_started_us = esp_timer_get_time();
    err = discard_frames(CAMERA_POST_BLC_CONVERGENCE_FRAMES,
                         "AEC/AWB convergence");
    if (err != ESP_OK) {
        goto cleanup;
    }
    ESP_LOGI(TAG, "Camera timing AEC/AWB=%0.1f ms total=%0.1f ms",
             (double)(esp_timer_get_time() - stage_started_us) / 1000.0,
             (double)(esp_timer_get_time() - capture_started_us) / 1000.0);

    ESP_LOGI(TAG,
             "QXGA timing: sysclk=%u HTS=%u VTS=%u max_exposure=%u gain_ceiling=0x%03x",
             (unsigned)timing.sysclk_hz,
             (unsigned)timing.hts_lines,
             (unsigned)timing.vts_lines,
             (unsigned)timing.max_exposure_lines,
             CAMERA_GAIN_CEILING);

    stage_started_us = esp_timer_get_time();
    frame = esp_camera_fb_get();
    if (!jpeg_has_markers(frame) || frame->width != 2048 || frame->height != 1536) {
        ESP_LOGE(TAG, "QXGA capture failed or returned invalid JPEG");
        err = ESP_FAIL;
        goto cleanup;
    }

    result->data = heap_caps_malloc(frame->len,
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!result->data) {
        ESP_LOGE(TAG, "Could not allocate %u-byte PSRAM JPEG copy",
                 (unsigned)frame->len);
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    memcpy(result->data, frame->buf, frame->len);
    result->size = frame->len;
    result->width = frame->width;
    result->height = frame->height;
    ESP_LOGI(TAG, "Camera timing final-frame-and-copy=%0.1f ms total=%0.1f ms",
             (double)(esp_timer_get_time() - stage_started_us) / 1000.0,
             (double)(esp_timer_get_time() - capture_started_us) / 1000.0);
    ESP_LOGI(TAG,
             "Copied QXGA JPEG to owned PSRAM: %ux%u len=%u exposure=%02x/%02x/%02x gain=%02x/%02x",
             (unsigned)frame->width, (unsigned)frame->height,
             (unsigned)frame->len,
             sensor->get_reg(sensor, 0x3500, 0x0f),
             sensor->get_reg(sensor, 0x3501, 0xff),
             sensor->get_reg(sensor, 0x3502, 0xf0),
             sensor->get_reg(sensor, 0x350a, 0x03),
             sensor->get_reg(sensor, 0x350b, 0xff));
    err = ESP_OK;

cleanup:
    if (frame) {
        esp_camera_fb_return(frame);
    }
    if (camera_initialized) {
        esp_err_t deinit_err = esp_camera_deinit();
        if (err == ESP_OK && deinit_err != ESP_OK) {
            err = deinit_err;
        }
    }
    esp_err_t power_err = camera_power_disable();
    if (err == ESP_OK && power_err != ESP_OK) {
        err = power_err;
    }
    if (err != ESP_OK) {
        camera_capture_release(result);
    }
    ESP_LOGI(TAG,
             "Camera frame returned, driver stopped, and GPIO42/U9 disabled");
    return err;
}
