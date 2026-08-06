#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ring_animation_start(uint32_t fade_in_ms, uint32_t hold_ms,
                               uint32_t fade_out_ms);
void ring_fade_stop(void);

#ifdef __cplusplus
}
#endif
