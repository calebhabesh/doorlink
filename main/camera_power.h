#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Rev C camera rail sequencing.
 *
 * camera_power_enable() first places the sensor controls in their safe
 * pre-power state, refuses to proceed while AMP_EN is high, then enables U9
 * and waits for CAM_3V3/+2V8/+1V5 to settle.
 * Call camera_power_disable() after the camera driver has been deinitialized.
 */
esp_err_t camera_power_prepare(void);
esp_err_t camera_power_enable(void);
esp_err_t camera_power_disable(void);

#ifdef __cplusplus
}
#endif
