#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "sensor.h"

#define OV5640_FULL_READOUT_BLC_FRAMES 4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t sysclk_hz;
    uint16_t hts_lines;
    uint16_t vts_lines;
    uint16_t max_exposure_lines;
    uint16_t band_step_50hz;
    uint16_t band_step_60hz;
    uint8_t max_bands_50hz;
    uint8_t max_bands_60hz;
} ov5640_aec_timing_t;

bool ov5640_uses_full_readout(framesize_t framesize);

esp_err_t ov5640_apply_full_readout_fix(sensor_t *sensor,
                                        framesize_t framesize,
                                        uint8_t blc_frames,
                                        bool continuous_blc_update);

esp_err_t ov5640_finish_blc_recalibration(sensor_t *sensor);

esp_err_t ov5640_set_auto_gain_ceiling(sensor_t *sensor,
                                       uint16_t gain_ceiling);

esp_err_t ov5640_configure_aec_timing(sensor_t *sensor,
                                      uint32_t xclk_hz,
                                      uint8_t mains_hz,
                                      uint16_t max_exposure_lines,
                                      ov5640_aec_timing_t *timing);

#ifdef __cplusplus
}
#endif
