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

/**
 * @brief Check if local chime playback (or retrigger window) is currently active.
 * @return true if currently playing or retriggering, false if idle.
 */
bool chime_player_is_playing(void);

/**
 * @brief Ask the asynchronous chime to stop at the next PCM chunk boundary.
 *
 * The amplifier is muted and the shared I2S bus is released before playback
 * finishes. Safe to call when no chime is active.
 */
void chime_player_request_stop(void);

/**
 * @brief Suppress local speaker chimes while visitor microphone capture owns audio.
 *
 * Enabling suppression stops any active chime and drops new/retrigger requests
 * instead of queueing them for delayed playback.
 */
void chime_player_set_suppressed(bool suppressed);

#ifdef __cplusplus
}
#endif
