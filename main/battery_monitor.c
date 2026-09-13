#include "battery_monitor.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "board_pins.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "battery_monitor";

#define BATTERY_SAMPLE_COUNT 16U
#define BATTERY_SAMPLE_INTERVAL_MS 2U
#define BATTERY_DIVIDER_RATIO 2U
#define BATTERY_PLAUSIBLE_MIN_MV 2500U
#define BATTERY_PLAUSIBLE_MAX_MV 5000U

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
        const esp_err_t err = adc_cali_delete_scheme_curve_fitting(handle);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "ADC calibration cleanup failed: %s",
                     esp_err_to_name(err));
        }
    }
#else
    (void)handle;
#endif
}

esp_err_t battery_monitor_read(battery_measurement_t *measurement)
{
    if (!measurement) {
        return ESP_ERR_INVALID_ARG;
    }
    *measurement = (battery_measurement_t){0};

    adc_unit_t unit;
    adc_channel_t channel;
    adc_oneshot_unit_handle_t adc_handle = NULL;
    adc_cali_handle_t calibration_handle = NULL;
    const adc_atten_t attenuation = ADC_ATTEN_DB_12;

    esp_err_t err = adc_oneshot_io_to_channel(
        BATTERY_ADC_PIN, &unit, &channel);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "GPIO%d ADC mapping failed: %s", BATTERY_ADC_PIN,
                 esp_err_to_name(err));
        return err;
    }

    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    err = adc_oneshot_new_unit(&unit_config, &adc_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ADC unit initialization failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = attenuation,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_oneshot_config_channel(adc_handle, channel, &channel_config);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ADC channel configuration failed: %s",
                 esp_err_to_name(err));
        goto cleanup;
    }

    err = create_calibration(
        unit, channel, attenuation, &calibration_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Calibrated battery voltage unavailable: %s",
                 esp_err_to_name(err));
        goto cleanup;
    }

    uint64_t raw_sum = 0;
    for (uint32_t i = 0; i < BATTERY_SAMPLE_COUNT; ++i) {
        int raw = 0;
        err = adc_oneshot_read(adc_handle, channel, &raw);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "ADC sample %" PRIu32 "/%u failed: %s",
                     i + 1U, BATTERY_SAMPLE_COUNT, esp_err_to_name(err));
            goto cleanup;
        }
        raw_sum += (uint32_t)raw;
        ++measurement->samples_read;
        if (i + 1U < BATTERY_SAMPLE_COUNT) {
            vTaskDelay(pdMS_TO_TICKS(BATTERY_SAMPLE_INTERVAL_MS));
        }
    }

    measurement->raw_average = (int)(
        (raw_sum + measurement->samples_read / 2U) /
        measurement->samples_read);
    int divider_mv = 0;
    err = adc_cali_raw_to_voltage(
        calibration_handle, measurement->raw_average, &divider_mv);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ADC voltage conversion failed: %s",
                 esp_err_to_name(err));
        goto cleanup;
    }

    measurement->millivolts = (uint32_t)divider_mv * BATTERY_DIVIDER_RATIO;
    if (measurement->millivolts < BATTERY_PLAUSIBLE_MIN_MV ||
        measurement->millivolts > BATTERY_PLAUSIBLE_MAX_MV) {
        ESP_LOGW(TAG, "Ignoring implausible battery reading: %" PRIu32 " mV",
                 measurement->millivolts);
        measurement->millivolts = 0;
        err = ESP_ERR_INVALID_RESPONSE;
        goto cleanup;
    }

    ESP_LOGI(TAG, "Battery=%" PRIu32 " mV (raw=%d, samples=%" PRIu32 ")",
             measurement->millivolts, measurement->raw_average,
             measurement->samples_read);

cleanup:
    delete_calibration(calibration_handle);
    if (adc_handle) {
        const esp_err_t delete_err = adc_oneshot_del_unit(adc_handle);
        if (delete_err != ESP_OK) {
            ESP_LOGW(TAG, "ADC unit cleanup failed: %s",
                     esp_err_to_name(delete_err));
            if (err == ESP_OK) {
                err = delete_err;
            }
        }
    }
    if (err != ESP_OK) {
        measurement->millivolts = 0;
    }
    return err;
}
