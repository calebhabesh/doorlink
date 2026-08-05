#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ring_fade_start(uint32_t fade_ms);
void ring_fade_stop(void);

#ifdef __cplusplus
}
#endif
