#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Fixed-point gain/envelope state shared by every production speaker path. */
typedef struct {
  int32_t target_gain_q15;
  size_t total_frames;
  size_t ramp_frames;
} speaker_envelope_t;

/** Initialize an envelope using the configured attenuation and ramp duration.
 */
void speaker_envelope_init(speaker_envelope_t *envelope,
                           uint32_t sample_rate_hz, size_t total_frames);

/** Initialize an envelope with an explicit experimental output profile. */
void speaker_envelope_init_profile(speaker_envelope_t *envelope,
                                   uint32_t sample_rate_hz,
                                   size_t total_frames,
                                   unsigned attenuation_db,
                                   unsigned ramp_ms);

/** Apply configured attenuation and the start/end ramp to one signed sample. */
int16_t speaker_envelope_apply(const speaker_envelope_t *envelope,
                               int16_t sample, size_t frame_index);

/** Configuration values exposed for startup/playback diagnostics. */
unsigned speaker_output_attenuation_db(void);
unsigned speaker_output_ramp_ms(void);

#ifdef __cplusplus
}
#endif
