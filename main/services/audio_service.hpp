#pragma once

#include <cstddef>
#include <cstdint>

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
    void stop();

private:
    esp_err_t start_microphone();

    i2s_chan_handle_t rx_channel_{nullptr};
    bool owns_audio_bus_{false};
};

}  // namespace doorbell
