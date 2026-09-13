#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt_client.h"
#include "services/intercom_protocol.hpp"

namespace doorbell {

enum class IntercomCommandType : std::uint8_t {
    StartReply,
    CancelReply,
    PlayAudio,
};

struct IntercomCommand {
    IntercomCommandType type;
    char command_id[37];
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
    static constexpr std::size_t kRecentCommandCount = 16;

    static void mqtt_event(void *args, esp_event_base_t base,
                           std::int32_t event_id, void *event_data);
    void handle_mqtt_event(esp_mqtt_event_handle_t event,
                           esp_mqtt_event_id_t event_id);
    void parse_command(const char *json);
    esp_err_t acknowledge(const char *url, std::int64_t deadline_us) const;

    esp_mqtt_client_handle_t mqtt_client_{nullptr};
    QueueHandle_t command_queue_{nullptr};
    char visitor_session_id_[33]{};
    char mqtt_payload_[1024]{};
    MqttPayloadAssembly mqtt_payload_assembly_{};
    char last_message_id_[37]{};
    char recent_command_ids_[kRecentCommandCount][37]{};
    std::size_t next_recent_command_id_{0};
    // Playback runs on the main task; retain its conversion buffers here.
    std::uint8_t playback_mono_[kPlaybackChunkBytes]{};
    std::int16_t playback_stereo_[kPlaybackChunkBytes]{};
};

}  // namespace doorbell
