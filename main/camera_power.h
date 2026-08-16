#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Rev C camera rail sequencing.
 *
 * camera_power_enable() first places the sensor controls in their safe
 * pre-power state. AMP_EN may remain high only when chime_player confirms the
 * bounded low-power overlap profile; rail enable is then staggered until its
 * ramp has settled. All other speaker ownership still blocks U9.
 * Call camera_power_disable() after the camera driver has been deinitialized.
 */
esp_err_t camera_power_prepare(void);
esp_err_t camera_power_enable(void);
esp_err_t camera_power_disable(void);
bool camera_power_is_enabled(void);

#ifdef __cplusplus
}
#endif
