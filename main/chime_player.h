#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Play the embedded doorbell chime sound asynchronously in a background task.
 * @return ESP_OK on task creation success, or error code.
 */
esp_err_t chime_player_play_async(void);

/**
 * @brief Play an interruptible repress chime only if the I2S bus is idle.
 *
 * Unlike the first-press chime, this request never waits behind microphone or
 * homeowner playback. If another local chime is already audible, its existing
 * I2S stream is rewound at the next PCM chunk boundary so every physical press
 * receives prompt audible acknowledgement.
 *
 * @return ESP_OK when playback was started, retriggered, or intentionally
 * suppressed; ESP_ERR_NO_MEM when the playback task could not be created.
 */
esp_err_t chime_player_play_repress_async(void);

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
 * @brief Check whether the active chime may yield to visitor/homeowner audio.
 */
bool chime_player_is_interruptible(void);

/**
 * @brief Prevent new/retriggered chimes while camera power is pending or on.
 *
 * An already-active chime is allowed to finish. After blocking new requests,
 * call chime_player_wait_until_idle() before enabling the camera rail.
 */
void chime_player_set_camera_power_blocked(bool blocked);

/**
 * @brief Wait until the amplifier has been muted and chime playback released.
 * @return ESP_OK when idle, ESP_ERR_TIMEOUT if timeout_ticks elapsed first.
 */
esp_err_t chime_player_wait_until_idle(TickType_t timeout_ticks);

/**
 * @brief Ask the asynchronous chime to stop at the next PCM chunk boundary.
 *
 * The amplifier is muted and the shared I2S bus is released before playback
 * finishes. Safe to call when no chime is active.
 */
void chime_player_request_stop(void);

#ifdef __cplusplus
}
#endif
