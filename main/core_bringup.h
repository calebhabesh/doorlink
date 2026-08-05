#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void core_bringup_configure_safe_gpio_state(bool camera_attached);
void run_core_bringup(bool camera_attached);

#ifdef __cplusplus
}
#endif
