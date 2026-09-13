#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t millivolts;
    int raw_average;
    uint32_t samples_read;
} battery_measurement_t;

// Takes a short, calibrated reading of +BATT through the populated 100k/100k
// divider on GPIO1. The caller should run this before enabling Wi-Fi or the
// camera so readings are taken at a consistent, low-load point in the cycle.
esp_err_t battery_monitor_read(battery_measurement_t *measurement);

#ifdef __cplusplus
}
#endif
