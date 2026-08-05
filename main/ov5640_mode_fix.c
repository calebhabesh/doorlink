#include "ov5640_mode_fix.h"

#include <limits.h>
#include <stddef.h>

/*
 * Espressif's OV5640 mode logic uses a 2560x1920 full-readout envelope and
 * bins only when both requested dimensions fit within half that size.
 */
bool ov5640_uses_full_readout(framesize_t framesize)
{
    if (framesize < 0 || framesize >= FRAMESIZE_INVALID) {
        return false;
    }

    return resolution[framesize].width > 1280 ||
           resolution[framesize].height > 960;
}

static int set_reg(sensor_t *sensor, int address, int mask, int value)
{
    return sensor->set_reg(sensor, address, mask, value);
}

esp_err_t ov5640_apply_full_readout_fix(sensor_t *sensor,
                                        framesize_t framesize,
                                        uint8_t blc_frames,
                                        bool continuous_blc_update)
{
    if (!sensor || sensor->id.PID != OV5640_PID) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (!ov5640_uses_full_readout(framesize)) {
        return ESP_OK;
    }

    int err = 0;

    /* Full-array analog/readout values used by the upstream QSXGA mode. */
    err |= set_reg(sensor, 0x3618, 0xff, 0x04);
    err |= set_reg(sensor, 0x3612, 0xff, 0x29);
    err |= set_reg(sensor, 0x3708, 0xff, 0x21);
    err |= set_reg(sensor, 0x3709, 0xff, 0x12);
    err |= set_reg(sensor, 0x370c, 0xff, 0x00);
    err |= set_reg(sensor, 0x4001, 0xff, 0x02);
    err |= set_reg(sensor, 0x4004, 0xff, 0x06);

    if (blc_frames > 0) {
        if (blc_frames > 0x3f) {
            blc_frames = 0x3f;
        }

        /* 0x4005[1]: update continuously; 0x4003[7]: redo BLC. */
        err |= set_reg(sensor, 0x4005, 0x02,
                       continuous_blc_update ? 0x02 : 0x00);
        err |= set_reg(sensor, 0x4003, 0xff, 0x80 | blc_frames);
    }

    return err == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t ov5640_finish_blc_recalibration(sensor_t *sensor)
{
    if (!sensor || sensor->id.PID != OV5640_PID) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    return set_reg(sensor, 0x4003, 0x80, 0x00) == 0 ? ESP_OK : ESP_FAIL;
}

static int read_reg(sensor_t *sensor, int address, int mask)
{
    return sensor->get_reg(sensor, address, mask);
}

static esp_err_t read_reg16(sensor_t *sensor, int address, uint16_t *value)
{
    int high = read_reg(sensor, address, 0xff);
    int low = read_reg(sensor, address + 1, 0xff);
    if (high < 0 || low < 0) {
        return ESP_FAIL;
    }

    *value = (uint16_t)(((uint16_t)high << 8) | (uint16_t)low);
    return ESP_OK;
}

static int write_reg16(sensor_t *sensor, int address, uint16_t value)
{
    return set_reg(sensor, address, 0xff, value >> 8) |
           set_reg(sensor, address + 1, 0xff, value & 0xff);
}

static esp_err_t calculate_sysclk(sensor_t *sensor,
                                  uint32_t xclk_hz,
                                  uint32_t *sysclk_hz)
{
    int bit_mode = read_reg(sensor, 0x3034, 0x0f);
    int sys_div_reg = read_reg(sensor, 0x3035, 0xff);
    int multiplier = read_reg(sensor, 0x3036, 0xff);
    int pre_div_reg = read_reg(sensor, 0x3037, 0xff);
    if (bit_mode < 0 || sys_div_reg < 0 || multiplier <= 0 ||
        pre_div_reg < 0 || xclk_hz == 0) {
        return ESP_FAIL;
    }

    int bit_div2x = 1;
    if (bit_mode == 8 || bit_mode == 10) {
        bit_div2x = bit_mode / 2;
    }

    int sys_div = sys_div_reg >> 4;
    if (sys_div == 0) {
        sys_div = 1;
    }

    /* The PLL pre-divider contains fractional choices encoded in half-steps. */
    static const uint8_t pre_div2x_map[] = {2, 2, 4, 6, 8, 3, 12, 5, 16};
    int pre_div_index = pre_div_reg & 0x0f;
    if (pre_div_index >= (int)(sizeof(pre_div2x_map) / sizeof(pre_div2x_map[0]))) {
        return ESP_ERR_INVALID_STATE;
    }

    int pre_div2x = pre_div2x_map[pre_div_index];
    int pll_root_div = ((pre_div_reg >> 4) & 0x01) + 1;
    uint64_t numerator = (uint64_t)xclk_hz * (uint32_t)multiplier;
    uint64_t denominator = (uint64_t)pre_div2x * (uint32_t)sys_div *
                           (uint32_t)pll_root_div * (uint32_t)bit_div2x;
    uint64_t result = numerator / denominator;
    if (result == 0 || result > UINT32_MAX) {
        return ESP_ERR_INVALID_STATE;
    }

    *sysclk_hz = (uint32_t)result;
    return ESP_OK;
}

esp_err_t ov5640_set_auto_gain_ceiling(sensor_t *sensor,
                                       uint16_t gain_ceiling)
{
    if (!sensor || sensor->id.PID != OV5640_PID) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (gain_ceiling < 0x10 || gain_ceiling > 0x03ff) {
        return ESP_ERR_INVALID_ARG;
    }

    int err = 0;
    err |= set_reg(sensor, 0x3a18, 0x03, gain_ceiling >> 8);
    err |= set_reg(sensor, 0x3a19, 0xff, gain_ceiling & 0xff);
    return err == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t ov5640_configure_aec_timing(sensor_t *sensor,
                                      uint32_t xclk_hz,
                                      uint8_t mains_hz,
                                      uint16_t max_exposure_lines,
                                      ov5640_aec_timing_t *timing)
{
    if (!sensor || sensor->id.PID != OV5640_PID) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (mains_hz != 50 && mains_hz != 60) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t sysclk_hz = 0;
    uint16_t hts = 0;
    uint16_t vts = 0;
    esp_err_t err = calculate_sysclk(sensor, xclk_hz, &sysclk_hz);
    if (err != ESP_OK || read_reg16(sensor, 0x380c, &hts) != ESP_OK ||
        read_reg16(sensor, 0x380e, &vts) != ESP_OK || hts == 0 || vts <= 4) {
        return err == ESP_OK ? ESP_FAIL : err;
    }

    uint32_t step_50 = (sysclk_hz + (uint32_t)hts * 50U) /
                       ((uint32_t)hts * 100U);
    uint32_t step_60 = (sysclk_hz + (uint32_t)hts * 60U) /
                       ((uint32_t)hts * 120U);
    if (step_50 == 0 || step_50 > 0x03ff ||
        step_60 == 0 || step_60 > 0x3fff) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t frame_limit = vts - 4;
    if (max_exposure_lines == 0 || max_exposure_lines > frame_limit) {
        max_exposure_lines = frame_limit;
    }

    uint32_t max_50 = max_exposure_lines / step_50;
    uint32_t max_60 = max_exposure_lines / step_60;
    if (max_50 == 0) {
        max_50 = 1;
    } else if (max_50 > 0x3f) {
        max_50 = 0x3f;
    }
    if (max_60 == 0) {
        max_60 = 1;
    } else if (max_60 > 0x3f) {
        max_60 = 0x3f;
    }

    int write_err = 0;
    write_err |= write_reg16(sensor, 0x3a02, max_exposure_lines);
    write_err |= write_reg16(sensor, 0x3a14, max_exposure_lines);
    write_err |= set_reg(sensor, 0x3a08, 0x03, step_50 >> 8);
    write_err |= set_reg(sensor, 0x3a09, 0xff, step_50 & 0xff);
    write_err |= set_reg(sensor, 0x3a0a, 0x3f, step_60 >> 8);
    write_err |= set_reg(sensor, 0x3a0b, 0xff, step_60 & 0xff);
    write_err |= set_reg(sensor, 0x3a0d, 0x3f, max_60);
    write_err |= set_reg(sensor, 0x3a0e, 0x3f, max_50);

    /* Force the known local mains frequency and keep multi-frame night mode off. */
    write_err |= set_reg(sensor, 0x3c01, 0x80, 0x80);
    write_err |= set_reg(sensor, 0x3c00, 0x04,
                         mains_hz == 50 ? 0x04 : 0x00);
    write_err |= set_reg(sensor, 0x3a00, 0x04, 0x00);
    if (write_err != 0) {
        return ESP_FAIL;
    }

    if (timing) {
        *timing = (ov5640_aec_timing_t) {
            .sysclk_hz = sysclk_hz,
            .hts_lines = hts,
            .vts_lines = vts,
            .max_exposure_lines = max_exposure_lines,
            .band_step_50hz = step_50,
            .band_step_60hz = step_60,
            .max_bands_50hz = max_50,
            .max_bands_60hz = max_60,
        };
    }

    return ESP_OK;
}
