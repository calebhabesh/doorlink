#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>

#include "driver/i2s_std.h"
#include "esp_err.h"

namespace doorbell {

class RecordedAudio final {
public:
    RecordedAudio() = default;
    ~RecordedAudio();

    RecordedAudio(const RecordedAudio &) = delete;
    RecordedAudio &operator=(const RecordedAudio &) = delete;

    const std::uint8_t *data() const { return data_; }
    std::size_t size() const { return size_; }
    bool valid() const { return data_ != nullptr && size_ > 44; }
    void reset();

private:
    friend class AudioService;
    std::uint8_t *data_{nullptr};
    std::size_t size_{0};
};

class AudioService final {
public:
    AudioService() = default;
    ~AudioService();

    AudioService(const AudioService &) = delete;
    AudioService &operator=(const AudioService &) = delete;

    esp_err_t record_greeting(RecordedAudio &audio, std::uint32_t duration_ms);
    esp_err_t record_button_hold(RecordedAudio &audio,
                                 std::uint32_t max_duration_ms,
                                 std::uint32_t minimum_duration_ms,
                                 std::uint32_t &recorded_duration_ms,
                                 std::int64_t absolute_deadline_us = 0);
    void stop();
    void request_capture_stop() { stop_requested_.store(true); }
    bool capture_active() const { return capture_active_.load(); }

private:
    static constexpr std::size_t kReadFrames = 256;

    esp_err_t start_microphone(std::int64_t absolute_deadline_us = 0);

    i2s_chan_handle_t rx_channel_{nullptr};
    bool owns_audio_bus_{false};
    std::atomic_bool capture_active_{false};
    std::atomic_bool stop_requested_{false};
    // I2S scratch storage must not consume the ESP-IDF main-task stack.
    std::int32_t sample_slots_[kReadFrames * 2]{};
};

}  // namespace doorbell
