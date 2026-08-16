#include "speaker_output.h"

#include <stdint.h>

#include "sdkconfig.h"

#ifndef CONFIG_SMART_DOORBELL_SPEAKER_ATTENUATION_DB
#define CONFIG_SMART_DOORBELL_SPEAKER_ATTENUATION_DB 6
#endif

#ifndef CONFIG_SMART_DOORBELL_SPEAKER_RAMP_MS
#define CONFIG_SMART_DOORBELL_SPEAKER_RAMP_MS 25
#endif

#define Q15_UNITY 32767

/* 10^(-dB/20), converted to Q15 for every supported integer dB setting. */
static const int32_t s_attenuation_q15[] = {
    32767, 29204, 26028, 23197, 20675, 18426, 16422, 14636, 13045,
    11626, 10362, 9235,  8231,  7336,  6538,  5827,  5193,  4628,
    4125,  3677,  3277,  2920,  2603,  2320,  2067,
};

unsigned speaker_output_attenuation_db(void) {
  return CONFIG_SMART_DOORBELL_SPEAKER_ATTENUATION_DB;
}

unsigned speaker_output_ramp_ms(void) {
  return CONFIG_SMART_DOORBELL_SPEAKER_RAMP_MS;
}

void speaker_envelope_init(speaker_envelope_t *envelope,
                           uint32_t sample_rate_hz, size_t total_frames) {
  speaker_envelope_init_profile(envelope, sample_rate_hz, total_frames,
                                speaker_output_attenuation_db(),
                                speaker_output_ramp_ms());
}

void speaker_envelope_init_profile(speaker_envelope_t *envelope,
                                   uint32_t sample_rate_hz,
                                   size_t total_frames,
                                   unsigned attenuation_db,
                                   unsigned ramp_ms) {
  if (envelope == NULL) {
    return;
  }

  const unsigned max_db =
      (unsigned)(sizeof(s_attenuation_q15) / sizeof(s_attenuation_q15[0]) - 1U);
  if (attenuation_db > max_db) {
    attenuation_db = max_db;
  }

  envelope->target_gain_q15 = s_attenuation_q15[attenuation_db];
  envelope->total_frames = total_frames;
  envelope->ramp_frames =
      ((size_t)sample_rate_hz * ramp_ms) / 1000U;
}

int16_t speaker_envelope_apply(const speaker_envelope_t *envelope,
                               int16_t sample, size_t frame_index) {
  if (envelope == NULL || envelope->total_frames == 0U ||
      frame_index >= envelope->total_frames) {
    return 0;
  }

  int32_t envelope_q15 = Q15_UNITY;
  if (envelope->ramp_frames > 0U) {
    if (frame_index < envelope->ramp_frames) {
      envelope_q15 =
          (int32_t)((frame_index * Q15_UNITY) / envelope->ramp_frames);
    }

    const size_t frames_after = envelope->total_frames - frame_index - 1U;
    if (frames_after < envelope->ramp_frames) {
      const int32_t fade_out_q15 =
          (int32_t)((frames_after * Q15_UNITY) / envelope->ramp_frames);
      if (fade_out_q15 < envelope_q15) {
        envelope_q15 = fade_out_q15;
      }
    }
  }

  const int32_t gain_q15 =
      (envelope->target_gain_q15 * envelope_q15) / Q15_UNITY;
  return (int16_t)(((int32_t)sample * gain_q15) / Q15_UNITY);
}
