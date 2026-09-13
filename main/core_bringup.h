#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void core_bringup_configure_safe_gpio_state(bool camera_attached);
esp_err_t core_bringup_hold_safe_gpio_state(void);
esp_err_t core_bringup_release_safe_gpio_holds(void);
void run_core_bringup(bool camera_attached);

#ifdef __cplusplus
}
#endif
