#include "battery_bringup.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "board_pins.h"
#include "core_bringup.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "battery_bringup";

#define BATTERY_SAMPLE_COUNT 64U
#define BATTERY_SAMPLE_INTERVAL_MS 5U
#define BATTERY_DIVIDER_NUMERATOR 2U
#define BATTERY_DIVIDER_DENOMINATOR 1U

static esp_err_t create_calibration(adc_unit_t unit,
                                    adc_channel_t channel,
                                    adc_atten_t attenuation,
                                    adc_cali_handle_t *handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    const adc_cali_curve_fitting_config_t config = {
        .unit_id = unit,
        .chan = channel,
        .atten = attenuation,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    return adc_cali_create_scheme_curve_fitting(&config, handle);
#else
    (void)unit;
    (void)channel;
    (void)attenuation;
    (void)handle;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static void delete_calibration(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (handle) {
        esp_err_t err = adc_cali_delete_scheme_curve_fitting(handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ADC calibration cleanup failed: %s",
                     esp_err_to_name(err));
        }
    }
#else
    (void)handle;
#endif
}

void run_battery_bringup(void)
{
    adc_unit_t unit;
    adc_channel_t channel;
    adc_oneshot_unit_handle_t adc_handle = NULL;
    adc_cali_handle_t calibration_handle = NULL;
    const adc_atten_t attenuation = ADC_ATTEN_DB_12;
    bool raw_result_valid = false;
    bool voltage_result_valid = false;
    int result_raw_average = 0;
    uint32_t result_battery_mv = 0;

    core_bringup_configure_safe_gpio_state(false);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGW(TAG, "REV C BATTERY-VOLTAGE DIAGNOSTIC");
    ESP_LOGI(TAG, "Camera, audio, amplifier, Wi-Fi, MQTT, and sleep are disabled");
    ESP_LOGW(TAG, "This image assumes the external camera, speaker, button, and PIR are disconnected");
    ESP_LOGW(TAG, "Firmware cannot disable or temperature-monitor the MCP73871 hardware charger");
    ESP_LOGI(TAG, "This reads +BATT through R7/R8 (100k/100k) on GPIO%d; it cannot read J5 thermistor temperature",
             BATTERY_ADC_PIN);

    esp_err_t err = adc_oneshot_io_to_channel(BATTERY_ADC_PIN, &unit, &channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO%d ADC mapping failed: %s",
                 BATTERY_ADC_PIN, esp_err_to_name(err));
        goto hold_safe;
    }

    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    err = adc_oneshot_new_unit(&unit_config, &adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC unit initialization failed: %s", esp_err_to_name(err));
        goto hold_safe;
    }

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = attenuation,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_oneshot_config_channel(adc_handle, channel, &channel_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC channel configuration failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    err = create_calibration(unit, channel, attenuation, &calibration_handle);
    const bool calibrated = err == ESP_OK;
    if (!calibrated) {
        ESP_LOGW(TAG, "ADC eFuse calibration unavailable (%s); raw counts only",
                 esp_err_to_name(err));
    }

    uint64_t raw_sum = 0;
    int raw_min = INT32_MAX;
    int raw_max = INT32_MIN;
    uint32_t samples_read = 0;

    for (uint32_t i = 0; i < BATTERY_SAMPLE_COUNT; ++i) {
        int raw = 0;
        err = adc_oneshot_read(adc_handle, channel, &raw);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ADC sample %" PRIu32 "/%u failed: %s",
                     i + 1U, BATTERY_SAMPLE_COUNT, esp_err_to_name(err));
            break;
        }

        raw_sum += (uint32_t)raw;
        if (raw < raw_min) {
            raw_min = raw;
        }
        if (raw > raw_max) {
            raw_max = raw;
        }
        ++samples_read;
        vTaskDelay(pdMS_TO_TICKS(BATTERY_SAMPLE_INTERVAL_MS));
    }

    if (samples_read == 0) {
        ESP_LOGE(TAG, "Battery measurement failed: no ADC samples");
        goto cleanup;
    }

    const int raw_average = (int)((raw_sum + samples_read / 2U) / samples_read);
    result_raw_average = raw_average;
    raw_result_valid = true;
    ESP_LOGI(TAG,
             "ADC%d channel=%d GPIO=%d samples=%" PRIu32 " raw min/avg/max=%d/%d/%d",
             (int)unit + 1, (int)channel, BATTERY_ADC_PIN, samples_read,
             raw_min, raw_average, raw_max);

    if (calibrated) {
        int divider_mv = 0;
        err = adc_cali_raw_to_voltage(
            calibration_handle, raw_average, &divider_mv);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ADC voltage conversion failed: %s",
                     esp_err_to_name(err));
        } else {
            result_battery_mv =
                ((uint32_t)divider_mv * BATTERY_DIVIDER_NUMERATOR +
                 BATTERY_DIVIDER_DENOMINATOR / 2U) /
                BATTERY_DIVIDER_DENOMINATOR;
            voltage_result_valid = true;
            ESP_LOGI(TAG,
                     "Calibrated ADC=%d mV; nominal divider battery=%" PRIu32 " mV (%" PRIu32 ".%03" PRIu32 " V)",
                     divider_mv, result_battery_mv,
                     result_battery_mv / 1000U, result_battery_mv % 1000U);
            ESP_LOGW(TAG,
                     "Record TP4 with a multimeter at the same time; do not use this first reading for a low-battery cutoff");
        }
    }

cleanup:
    delete_calibration(calibration_handle);
    if (adc_handle) {
        esp_err_t delete_err = adc_oneshot_del_unit(adc_handle);
        if (delete_err != ESP_OK) {
            ESP_LOGE(TAG, "ADC unit cleanup failed: %s",
                     esp_err_to_name(delete_err));
        }
    }

hold_safe:
    ESP_LOGI(TAG, "Bounded ADC attempt complete; safe GPIO state remains active");
    for (uint32_t heartbeat = 0;; ++heartbeat) {
        if (voltage_result_valid) {
            ESP_LOGI(TAG,
                     "retained result: raw=%d battery=%" PRIu32 " mV (%" PRIu32 ".%03" PRIu32 " V); reset to resample",
                     result_raw_average, result_battery_mv,
                     result_battery_mv / 1000U, result_battery_mv % 1000U);
        } else if (raw_result_valid) {
            ESP_LOGI(TAG,
                     "retained uncalibrated result: raw=%d; reset to resample",
                     result_raw_average);
        } else {
            ESP_LOGE(TAG,
                     "heartbeat %" PRIu32 "; no retained ADC result; reset to retry",
                     heartbeat);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
