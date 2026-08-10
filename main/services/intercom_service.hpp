#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt_client.h"

namespace doorbell {

enum class IntercomCommandType : std::uint8_t {
    StartReply,
    CancelReply,
    PlayAudio,
};

struct IntercomCommand {
    IntercomCommandType type;
    char event_id[65];
    char message_id[37];
    char audio_url[320];
    char ack_url[320];
};

class IntercomService final {
public:
    IntercomService() = default;
    ~IntercomService();

    IntercomService(const IntercomService &) = delete;
    IntercomService &operator=(const IntercomService &) = delete;

    esp_err_t start(const char *visitor_session_id);
    bool receive(IntercomCommand &command, TickType_t timeout_ticks);
    esp_err_t play_wav(const IntercomCommand &command,
                       std::uint32_t max_duration_ms);
    void stop();

private:
    static constexpr std::size_t kPlaybackChunkBytes = 1024;

    static void mqtt_event(void *args, esp_event_base_t base,
                           std::int32_t event_id, void *event_data);
    void handle_mqtt_event(esp_mqtt_event_handle_t event,
                           esp_mqtt_event_id_t event_id);
    void parse_command(const char *json);
    esp_err_t acknowledge(const char *url) const;

    esp_mqtt_client_handle_t mqtt_client_{nullptr};
    QueueHandle_t command_queue_{nullptr};
    char visitor_session_id_[33]{};
    char mqtt_payload_[1024]{};
    std::size_t mqtt_payload_size_{0};
    // Playback runs on the main task; retain its conversion buffers here.
    std::uint8_t playback_mono_[kPlaybackChunkBytes]{};
    std::int16_t playback_stereo_[kPlaybackChunkBytes]{};
};

}  // namespace doorbell
