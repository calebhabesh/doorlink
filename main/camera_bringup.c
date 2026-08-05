#include "camera_bringup.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "board_pins.h"
#include "camera_power.h"
#include "driver/gpio.h"
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/base64.h"
#include "ov5640_mode_fix.h"

static const char *TAG = "camera_bringup";

#ifndef CAMERA_BRINGUP_PREPARE_DELAY_MS
#define CAMERA_BRINGUP_PREPARE_DELAY_MS 5000
#endif

#ifndef CAMERA_BRINGUP_DUMP_JPEG_BASE64
#define CAMERA_BRINGUP_DUMP_JPEG_BASE64 0
#endif

#ifndef CAMERA_BRINGUP_FRAME_SIZE
#define CAMERA_BRINGUP_FRAME_SIZE FRAMESIZE_QQVGA
#endif

#ifndef CAMERA_BRINGUP_JPEG_QUALITY
#define CAMERA_BRINGUP_JPEG_QUALITY 20
#endif

#ifndef CAMERA_BRINGUP_POST_INIT_FRAME_SIZE
#define CAMERA_BRINGUP_POST_INIT_FRAME_SIZE FRAMESIZE_INVALID
#endif

#ifndef CAMERA_BRINGUP_XCLK_HZ
#define CAMERA_BRINGUP_XCLK_HZ 10000000
#endif

#ifndef CAMERA_BRINGUP_VFLIP
#define CAMERA_BRINGUP_VFLIP 0
#endif

#ifndef CAMERA_BRINGUP_HMIRROR
#define CAMERA_BRINGUP_HMIRROR 0
#endif

#ifndef CAMERA_BRINGUP_WARMUP_FRAMES
#define CAMERA_BRINGUP_WARMUP_FRAMES 0
#endif

#ifndef CAMERA_BRINGUP_COLORBAR
#define CAMERA_BRINGUP_COLORBAR 0
#endif

/*
 * OV5640 register 0x3824 is the manual DVP PCLK divider. The Espressif
 * driver selects 4 for JPEG modes. Leave this at 0 to retain that selection;
 * diagnostic builds may use 8 (or another 1..31 value) to change only the
 * output-interface clock without also changing XCLK and the sensor/ISP clocks.
 */
#ifndef CAMERA_BRINGUP_PCLK_DIV
#define CAMERA_BRINGUP_PCLK_DIV 0
#endif

/*
 * Espressif changes the OV5640 sampling increments for unbinned modes but
 * does not apply the corresponding full-readout analog/BLC recipe. Enable
 * this only in diagnostic builds while validating that mode-table delta.
 */
#ifndef CAMERA_BRINGUP_FULL_READOUT_TUNING
#define CAMERA_BRINGUP_FULL_READOUT_TUNING 0
#endif

/*
 * Trigger a fresh OV5640 black-level calibration after changing to the
 * unbinned readout recipe. The low six bits of 0x4003 select how many frames
 * calibration runs; bit 7 requests the redo. During this bounded diagnostic,
 * 0x4005 bit 1 keeps BLC updating rather than freezing after the first frame.
 */
#ifndef CAMERA_BRINGUP_BLC_RECALIBRATE_FRAMES
#define CAMERA_BRINGUP_BLC_RECALIBRATE_FRAMES 0
#endif

#ifndef CAMERA_BRINGUP_BLC_CONTINUOUS_UPDATE
#define CAMERA_BRINGUP_BLC_CONTINUOUS_UPDATE 1
#endif

/* Negative values preserve the sensor's automatic controls. */
#ifndef CAMERA_BRINGUP_MANUAL_GAIN
#define CAMERA_BRINGUP_MANUAL_GAIN -1
#endif

#ifndef CAMERA_BRINGUP_MANUAL_EXPOSURE
#define CAMERA_BRINGUP_MANUAL_EXPOSURE -1
#endif

#ifndef CAMERA_BRINGUP_FIXED_WB_MODE
#define CAMERA_BRINGUP_FIXED_WB_MODE -1
#endif

/*
 * Raw OV5640 automatic-gain ceiling in 1/16x units. A value of 0 disables
 * this diagnostic override. The esp32-camera gainceiling_t values are enum
 * indices, but the OV5640 0x3a18/0x3a19 registers require the encoded gain;
 * for example, 0x0020 is a 2x ceiling.
 */
#ifndef CAMERA_BRINGUP_AUTO_GAIN_CEILING
#define CAMERA_BRINGUP_AUTO_GAIN_CEILING 0
#endif

/* Set to 50 or 60 to recalculate OV5640 AEC timing from the active mode. */
#ifndef CAMERA_BRINGUP_AEC_MAINS_HZ
#define CAMERA_BRINGUP_AEC_MAINS_HZ 0
#endif

/* Zero uses the active frame's VTS minus the four-line sensor margin. */
#ifndef CAMERA_BRINGUP_AEC_MAX_EXPOSURE_LINES
#define CAMERA_BRINGUP_AEC_MAX_EXPOSURE_LINES 0
#endif

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
#if CAMERA_BRINGUP_DUMP_JPEG_BASE64
    unsigned char *jpeg_base64 = NULL;
    size_t jpeg_base64_len = 0;
#endif

    configure_safe_outputs();

    ESP_LOGW(TAG, "REV C ONE-FRAME CAMERA TEST: verified camera must be connected");
    ESP_LOGI(TAG, "Wi-Fi, mic, amplifier, MQTT, and sleep are disabled");
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
        .xclk_freq_hz = CAMERA_BRINGUP_XCLK_HZ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = CAMERA_BRINGUP_FRAME_SIZE,
        .jpeg_quality = CAMERA_BRINGUP_JPEG_QUALITY,
        .fb_count = 1,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = camera_power_prepare();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera safe-state preparation failed: %s",
                 esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Camera rails are off; one-frame test starts in %d ms",
                 CAMERA_BRINGUP_PREPARE_DELAY_MS);
        vTaskDelay(pdMS_TO_TICKS(CAMERA_BRINGUP_PREPARE_DELAY_MS));
        gpio_set_level(STATUS_LED_PIN, 1);
        err = camera_power_enable();
    }

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
                if (CAMERA_BRINGUP_POST_INIT_FRAME_SIZE != FRAMESIZE_INVALID) {
                    int framesize_err = sensor->set_framesize(
                        sensor, CAMERA_BRINGUP_POST_INIT_FRAME_SIZE);
                    if (framesize_err != 0) {
                        ESP_LOGE(TAG, "Post-init frame-size selection failed: %d",
                                 framesize_err);
                    } else {
                        ESP_LOGI(TAG, "Post-init frame size selected: enum=%d",
                                 CAMERA_BRINGUP_POST_INIT_FRAME_SIZE);
                    }
                }

                int orientation_err = 0;
                if (CAMERA_BRINGUP_VFLIP) {
                    orientation_err |= sensor->set_vflip(sensor, 1);
                }
                if (CAMERA_BRINGUP_HMIRROR) {
                    orientation_err |= sensor->set_hmirror(sensor, 1);
                }
                if (orientation_err != 0) {
                    ESP_LOGE(TAG, "Sensor orientation correction failed: %d",
                             orientation_err);
                } else if (CAMERA_BRINGUP_VFLIP || CAMERA_BRINGUP_HMIRROR) {
                    ESP_LOGI(TAG, "Sensor orientation corrected: vflip=%d hmirror=%d",
                             CAMERA_BRINGUP_VFLIP, CAMERA_BRINGUP_HMIRROR);
                }

                if (CAMERA_BRINGUP_PCLK_DIV > 0) {
                    int pclk_err = sensor->set_reg(
                        sensor, 0x3824, 0x1f, CAMERA_BRINGUP_PCLK_DIV);
                    if (pclk_err != 0) {
                        ESP_LOGE(TAG, "OV5640 PCLK divider override failed: %d",
                                 pclk_err);
                    } else {
                        ESP_LOGI(TAG, "OV5640 manual PCLK divider overridden: %d",
                                 CAMERA_BRINGUP_PCLK_DIV);
                    }
                }

                if (CAMERA_BRINGUP_COLORBAR) {
                    int colorbar_err = sensor->set_colorbar(sensor, 1);
                    if (colorbar_err != 0) {
                        ESP_LOGE(TAG, "OV5640 color-bar enable failed: %d",
                                 colorbar_err);
                    } else {
                        ESP_LOGI(TAG, "OV5640 internal color bar enabled");
                    }
                }

                if (CAMERA_BRINGUP_FULL_READOUT_TUNING) {
                    esp_err_t tuning_err = ov5640_apply_full_readout_fix(
                        sensor,
                        sensor->status.framesize,
                        CAMERA_BRINGUP_BLC_RECALIBRATE_FRAMES,
                        CAMERA_BRINGUP_BLC_CONTINUOUS_UPDATE != 0);
                    if (tuning_err != ESP_OK) {
                        ESP_LOGE(TAG, "OV5640 full-readout tuning failed: %s",
                                 esp_err_to_name(tuning_err));
                    } else {
                        ESP_LOGI(TAG, "OV5640 full-readout analog/BLC tuning applied");
                        if (CAMERA_BRINGUP_BLC_RECALIBRATE_FRAMES > 0) {
                            ESP_LOGI(TAG,
                                     "OV5640 BLC recalibration requested: frames=%d "
                                     "continuous_update=%d",
                                     CAMERA_BRINGUP_BLC_RECALIBRATE_FRAMES,
                                     CAMERA_BRINGUP_BLC_CONTINUOUS_UPDATE ? 1 : 0);
                        }
                    }
                }

                if (CAMERA_BRINGUP_AEC_MAINS_HZ != 0) {
                    ov5640_aec_timing_t aec_timing = {0};
                    esp_err_t aec_err = ov5640_configure_aec_timing(
                        sensor,
                        CAMERA_BRINGUP_XCLK_HZ,
                        CAMERA_BRINGUP_AEC_MAINS_HZ,
                        CAMERA_BRINGUP_AEC_MAX_EXPOSURE_LINES,
                        &aec_timing);
                    if (aec_err != ESP_OK) {
                        ESP_LOGE(TAG, "OV5640 AEC timing setup failed: %s",
                                 esp_err_to_name(aec_err));
                    } else {
                        ESP_LOGI(TAG,
                                 "OV5640 AEC timing: mains=%dHz sysclk=%u "
                                 "HTS=%u VTS=%u max_exposure=%u "
                                 "B50=%u/%u B60=%u/%u",
                                 CAMERA_BRINGUP_AEC_MAINS_HZ,
                                 (unsigned)aec_timing.sysclk_hz,
                                 (unsigned)aec_timing.hts_lines,
                                 (unsigned)aec_timing.vts_lines,
                                 (unsigned)aec_timing.max_exposure_lines,
                                 (unsigned)aec_timing.band_step_50hz,
                                 (unsigned)aec_timing.max_bands_50hz,
                                 (unsigned)aec_timing.band_step_60hz,
                                 (unsigned)aec_timing.max_bands_60hz);
                    }
                }

                int manual_err = 0;
                if (CAMERA_BRINGUP_AUTO_GAIN_CEILING > 0) {
                    manual_err |= sensor->set_gain_ctrl(sensor, 1);
                    manual_err |= sensor->set_exposure_ctrl(sensor, 1);
                    manual_err |= sensor->set_whitebal(sensor, 1);
                    manual_err |= sensor->set_awb_gain(sensor, 1);
                    manual_err |= sensor->set_wb_mode(sensor, 0);
                    manual_err |= ov5640_set_auto_gain_ceiling(
                        sensor, CAMERA_BRINGUP_AUTO_GAIN_CEILING);
                }
                if (CAMERA_BRINGUP_MANUAL_GAIN >= 0) {
                    manual_err |= sensor->set_gain_ctrl(sensor, 0);
                    manual_err |= sensor->set_agc_gain(
                        sensor, CAMERA_BRINGUP_MANUAL_GAIN);
                }
                if (CAMERA_BRINGUP_MANUAL_EXPOSURE >= 0) {
                    manual_err |= sensor->set_exposure_ctrl(sensor, 0);
                    manual_err |= sensor->set_aec2(sensor, 0);
                    manual_err |= sensor->set_aec_value(
                        sensor, CAMERA_BRINGUP_MANUAL_EXPOSURE);
                }
                if (CAMERA_BRINGUP_FIXED_WB_MODE >= 0) {
                    manual_err |= sensor->set_whitebal(sensor, 0);
                    manual_err |= sensor->set_awb_gain(sensor, 0);
                    manual_err |= sensor->set_wb_mode(
                        sensor, CAMERA_BRINGUP_FIXED_WB_MODE);
                }
                if (manual_err != 0) {
                    ESP_LOGE(TAG, "OV5640 manual image controls failed: %d",
                             manual_err);
                } else if (CAMERA_BRINGUP_MANUAL_GAIN >= 0 ||
                           CAMERA_BRINGUP_MANUAL_EXPOSURE >= 0 ||
                           CAMERA_BRINGUP_FIXED_WB_MODE >= 0 ||
                           CAMERA_BRINGUP_AUTO_GAIN_CEILING > 0) {
                    ESP_LOGI(TAG,
                             "OV5640 manual controls: gain=%d exposure=%d "
                             "wb_mode=%d auto_gain_ceiling=0x%03x",
                             CAMERA_BRINGUP_MANUAL_GAIN,
                             CAMERA_BRINGUP_MANUAL_EXPOSURE,
                             CAMERA_BRINGUP_FIXED_WB_MODE,
                             CAMERA_BRINGUP_AUTO_GAIN_CEILING);
                }

                ESP_LOGI(TAG,
                         "OV5640 timing regs: PLL=%02x/%02x/%02x/%02x root=%02x "
                         "PCLK_DIV=%02x VFIFO=%02x POLARITY=%02x DRIVE=%02x",
                         sensor->get_reg(sensor, 0x3034, 0xff),
                         sensor->get_reg(sensor, 0x3035, 0xff),
                         sensor->get_reg(sensor, 0x3036, 0xff),
                         sensor->get_reg(sensor, 0x3037, 0xff),
                         sensor->get_reg(sensor, 0x3108, 0xff),
                         sensor->get_reg(sensor, 0x3824, 0x1f),
                         sensor->get_reg(sensor, 0x460c, 0xff),
                         sensor->get_reg(sensor, 0x4740, 0xff),
                         sensor->get_reg(sensor, 0x302c, 0xff));
                ESP_LOGI(TAG,
                         "OV5640 readout regs: 3618=%02x 3612=%02x 3708=%02x "
                         "3709=%02x 370c=%02x BLC=%02x/%02x/%02x/%02x/%02x/%02x",
                         sensor->get_reg(sensor, 0x3618, 0xff),
                         sensor->get_reg(sensor, 0x3612, 0xff),
                         sensor->get_reg(sensor, 0x3708, 0xff),
                         sensor->get_reg(sensor, 0x3709, 0xff),
                         sensor->get_reg(sensor, 0x370c, 0xff),
                         sensor->get_reg(sensor, 0x4000, 0xff),
                         sensor->get_reg(sensor, 0x4001, 0xff),
                         sensor->get_reg(sensor, 0x4002, 0xff),
                         sensor->get_reg(sensor, 0x4003, 0xff),
                         sensor->get_reg(sensor, 0x4004, 0xff),
                         sensor->get_reg(sensor, 0x4005, 0xff));
            }

            bool warmup_ok = true;
            for (int i = 0; i < CAMERA_BRINGUP_WARMUP_FRAMES; ++i) {
                camera_fb_t *warmup_fb = esp_camera_fb_get();
                if (!warmup_fb) {
                    ESP_LOGE(TAG, "Warm-up frame %d/%d failed",
                             i + 1, CAMERA_BRINGUP_WARMUP_FRAMES);
                    warmup_ok = false;
                    break;
                }
                ESP_LOGI(TAG, "Discarding warm-up frame %d/%d: len=%u",
                         i + 1, CAMERA_BRINGUP_WARMUP_FRAMES,
                         (unsigned)warmup_fb->len);
                esp_camera_fb_return(warmup_fb);
            }

            if (warmup_ok && CAMERA_BRINGUP_FULL_READOUT_TUNING &&
                CAMERA_BRINGUP_BLC_RECALIBRATE_FRAMES > 0) {
                esp_err_t blc_finish_err =
                    ov5640_finish_blc_recalibration(sensor);
                if (blc_finish_err != ESP_OK) {
                    ESP_LOGE(TAG, "OV5640 BLC recalibration finish failed: %s",
                             esp_err_to_name(blc_finish_err));
                    warmup_ok = false;
                } else {
                    ESP_LOGI(TAG,
                             "OV5640 BLC recalibration finished: redo_bit=%02x",
                             sensor->get_reg(sensor, 0x4003, 0x80));
                }
            }

            if (sensor) {
                ESP_LOGI(TAG,
                         "OV5640 capture controls: manual=%02x "
                         "exposure=%02x/%02x/%02x gain=%02x/%02x "
                         "BLC_TARGET=%02x",
                         sensor->get_reg(sensor, 0x3503, 0xff),
                         sensor->get_reg(sensor, 0x3500, 0x0f),
                         sensor->get_reg(sensor, 0x3501, 0xff),
                         sensor->get_reg(sensor, 0x3502, 0xf0),
                         sensor->get_reg(sensor, 0x350a, 0x03),
                         sensor->get_reg(sensor, 0x350b, 0xff),
                         sensor->get_reg(sensor, 0x4009, 0xff));
                ESP_LOGI(TAG,
                         "OV5640 automatic controls: gain_ceiling=%02x/%02x "
                         "aec_max=%02x/%02x B50=%02x/%02x/%02x "
                         "B60=%02x/%02x/%02x freq=%02x/%02x "
                         "awb_manual=%02x awb_gain=%02x/%02x/%02x/%02x/%02x/%02x",
                         sensor->get_reg(sensor, 0x3a18, 0x03),
                         sensor->get_reg(sensor, 0x3a19, 0xff),
                         sensor->get_reg(sensor, 0x3a02, 0xff),
                         sensor->get_reg(sensor, 0x3a03, 0xff),
                         sensor->get_reg(sensor, 0x3a08, 0x03),
                         sensor->get_reg(sensor, 0x3a09, 0xff),
                         sensor->get_reg(sensor, 0x3a0e, 0x3f),
                         sensor->get_reg(sensor, 0x3a0a, 0x3f),
                         sensor->get_reg(sensor, 0x3a0b, 0xff),
                         sensor->get_reg(sensor, 0x3a0d, 0x3f),
                         sensor->get_reg(sensor, 0x3c01, 0x80),
                         sensor->get_reg(sensor, 0x3c00, 0x04),
                         sensor->get_reg(sensor, 0x3406, 0x01),
                         sensor->get_reg(sensor, 0x3400, 0xff),
                         sensor->get_reg(sensor, 0x3401, 0xff),
                         sensor->get_reg(sensor, 0x3402, 0xff),
                         sensor->get_reg(sensor, 0x3403, 0xff),
                         sensor->get_reg(sensor, 0x3404, 0xff),
                         sensor->get_reg(sensor, 0x3405, 0xff));
            }

            camera_fb_t *fb = warmup_ok ? esp_camera_fb_get() : NULL;
            if (!fb) {
                ESP_LOGE(TAG, "Frame capture failed");
            } else {
                ESP_LOGI(TAG, "Frame captured: %ux%u len=%u format=%d jpeg_markers=%s",
                         (unsigned)fb->width, (unsigned)fb->height, (unsigned)fb->len,
                         fb->format, jpeg_has_markers(fb) ? "valid" : "invalid");
#if CAMERA_BRINGUP_DUMP_JPEG_BASE64
                if (jpeg_has_markers(fb)) {
                    size_t capacity = 0;
                    mbedtls_base64_encode(NULL, 0, &capacity, fb->buf, fb->len);
                    jpeg_base64 = heap_caps_malloc(capacity, MALLOC_CAP_8BIT);
                    if (!jpeg_base64) {
                        ESP_LOGE(TAG, "Could not allocate %u-byte JPEG export buffer",
                                 (unsigned)capacity);
                    } else {
                        int base64_err = mbedtls_base64_encode(
                            jpeg_base64, capacity, &jpeg_base64_len, fb->buf, fb->len);
                        if (base64_err != 0) {
                            ESP_LOGE(TAG, "JPEG base64 encoding failed: -0x%04x",
                                     (unsigned)-base64_err);
                            heap_caps_free(jpeg_base64);
                            jpeg_base64 = NULL;
                            jpeg_base64_len = 0;
                        }
                    }
                }
#endif
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
    gpio_set_level(STATUS_LED_PIN, 0);

#if CAMERA_BRINGUP_DUMP_JPEG_BASE64
    if (jpeg_base64) {
        ESP_LOGI(TAG, "JPEG_BASE64_BEGIN encoded_len=%u; camera rails are off",
                 (unsigned)jpeg_base64_len);
        for (size_t offset = 0; offset < jpeg_base64_len; offset += 96) {
            size_t remaining = jpeg_base64_len - offset;
            int chunk_len = (int)(remaining < 96 ? remaining : 96);
            printf("JPEG64:%.*s\n", chunk_len, jpeg_base64 + offset);
        }
        printf("JPEG_BASE64_END\n");
        fflush(stdout);
        heap_caps_free(jpeg_base64);
    }
#endif

    for (uint32_t i = 0;; ++i) {
        gpio_set_level(STATUS_LED_PIN, i % 2);
        ESP_LOGI(TAG, "Camera bring-up heartbeat %u", (unsigned)i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
