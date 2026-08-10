#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

bool audio_bus_acquire(TickType_t timeout_ticks);
void audio_bus_release(void);

#ifdef __cplusplus
}
#endif
