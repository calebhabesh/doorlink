#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Play the embedded doorbell chime sound asynchronously in a background task.
 * @return ESP_OK on task creation success, or error code.
 */
esp_err_t chime_player_play_async(void);

/**
 * @brief Play the embedded doorbell chime sound synchronously (blocking until finished).
 * @return ESP_OK on playback success, or error code.
 */
esp_err_t chime_player_play_sync(void);

#ifdef __cplusplus
}
#endif
