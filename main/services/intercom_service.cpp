#include "services/intercom_service.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "audio_bus.h"
#include "board_pins.h"
#include "config.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "speaker_output.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/task.h"

namespace doorbell {
namespace {
constexpr const char *kTag = "IntercomService";
#ifndef MQTT_PTT_TOPIC
#define MQTT_PTT_TOPIC "doorbell/commands/audio"
#endif
constexpr const char *kCommandTopic = MQTT_PTT_TOPIC;
constexpr std::uint32_t kSampleRateHz = 16000;

bool topic_equals(const esp_mqtt_event_handle_t event, const char *topic)
{
    const std::size_t length = std::strlen(topic);
    return event->topic_len == static_cast<int>(length) &&
           std::memcmp(event->topic, topic, length) == 0;
}

std::uint32_t read_u32(const std::uint8_t *data)
{
    return static_cast<std::uint32_t>(data[0]) |
           (static_cast<std::uint32_t>(data[1]) << 8) |
           (static_cast<std::uint32_t>(data[2]) << 16) |
           (static_cast<std::uint32_t>(data[3]) << 24);
}

std::uint16_t read_u16(const std::uint8_t *data)
{
    return static_cast<std::uint16_t>(data[0]) |
           static_cast<std::uint16_t>(data[1] << 8);
}

bool copy_json_string(const char *json, const char *name, char *destination,
                      std::size_t destination_size)
{
    char key[48];
    const int key_length = std::snprintf(key, sizeof(key), "\"%s\"", name);
    if (key_length <= 0 || static_cast<std::size_t>(key_length) >= sizeof(key)) {
        return false;
    }
    const char *cursor = std::strstr(json, key);
    if (!cursor) return false;
    cursor = std::strchr(cursor + key_length, ':');
    if (!cursor) return false;
    cursor = std::strchr(cursor + 1, '"');
    if (!cursor) return false;
    ++cursor;
    const char *end = std::strchr(cursor, '"');
    if (!end || end == cursor) return false;
    const std::size_t length = static_cast<std::size_t>(end - cursor);
    if (length >= destination_size) return false;
    std::memcpy(destination, cursor, length);
    destination[length] = '\0';
    return true;
}

esp_err_t read_exact(esp_http_client_handle_t client, std::uint8_t *data,
                     std::size_t length)
{
    std::size_t offset = 0;
    while (offset < length) {
        const int read = esp_http_client_read(
            client, reinterpret_cast<char *>(data + offset), length - offset);
        if (read <= 0) {
            return ESP_FAIL;
        }
        offset += static_cast<std::size_t>(read);
    }
    return ESP_OK;
}
}  // namespace

IntercomService::~IntercomService()
{
    stop();
}

esp_err_t IntercomService::start(const char *visitor_session_id)
{
    if (mqtt_client_) {
        return ESP_OK;
    }
    if (!visitor_session_id || std::strlen(visitor_session_id) != 32) {
        return ESP_ERR_INVALID_ARG;
    }

    std::snprintf(visitor_session_id_, sizeof(visitor_session_id_), "%s",
                  visitor_session_id);
    command_queue_ = xQueueCreate(4, sizeof(IntercomCommand));
    if (!command_queue_) {
        return ESP_ERR_NO_MEM;
    }

    esp_mqtt_client_config_t config = {};
    config.broker.address.uri = MQTT_BROKER_URI;
    config.buffer.size = sizeof(mqtt_payload_);
    mqtt_client_ = esp_mqtt_client_init(&config);
    if (!mqtt_client_) {
        stop();
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_mqtt_client_register_event(
        mqtt_client_, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
        &IntercomService::mqtt_event, this);
    if (err == ESP_OK) {
        err = esp_mqtt_client_start(mqtt_client_);
    }
    if (err != ESP_OK) {
        stop();
        return err;
    }
    ESP_LOGI(kTag, "Intercom control plane starting for session %s",
             visitor_session_id_);
    return ESP_OK;
}

bool IntercomService::receive(IntercomCommand &command, TickType_t timeout_ticks)
{
    return command_queue_ &&
           xQueueReceive(command_queue_, &command, timeout_ticks) == pdTRUE;
}

void IntercomService::mqtt_event(void *args, esp_event_base_t,
                                 std::int32_t event_id, void *event_data)
{
    static_cast<IntercomService *>(args)->handle_mqtt_event(
        static_cast<esp_mqtt_event_handle_t>(event_data),
        static_cast<esp_mqtt_event_id_t>(event_id));
}

void IntercomService::handle_mqtt_event(esp_mqtt_event_handle_t event,
                                        esp_mqtt_event_id_t event_id)
{
    if (event_id == MQTT_EVENT_CONNECTED) {
        esp_mqtt_client_subscribe(event->client, kCommandTopic, 1);
        ESP_LOGI(kTag, "Subscribed to %s", kCommandTopic);
        return;
    }
    if (event_id != MQTT_EVENT_DATA ||
        (event->current_data_offset == 0 && !topic_equals(event, kCommandTopic))) {
        return;
    }

    if (event->current_data_offset == 0) {
        mqtt_payload_size_ = 0;
        if (event->total_data_len >= static_cast<int>(sizeof(mqtt_payload_))) {
            ESP_LOGW(kTag, "Discarding oversized intercom command (%d bytes)",
                     event->total_data_len);
            return;
        }
    }
    if (mqtt_payload_size_ + event->data_len >= sizeof(mqtt_payload_)) {
        mqtt_payload_size_ = 0;
        return;
    }
    std::memcpy(mqtt_payload_ + event->current_data_offset, event->data,
                event->data_len);
    mqtt_payload_size_ += event->data_len;
    if (event->current_data_offset + event->data_len == event->total_data_len) {
        mqtt_payload_[event->total_data_len] = '\0';
        parse_command(mqtt_payload_);
        mqtt_payload_size_ = 0;
    }
}

void IntercomService::parse_command(const char *json)
{
    IntercomCommand command{};
    char type[24]{};
    copy_json_string(json, "type", type, sizeof(type));
    copy_json_string(json, "eventId", command.event_id,
                     sizeof(command.event_id));
    copy_json_string(json, "messageId", command.message_id,
                     sizeof(command.message_id));
    copy_json_string(json, "audioUrl", command.audio_url,
                     sizeof(command.audio_url));
    copy_json_string(json, "ackUrl", command.ack_url,
                     sizeof(command.ack_url));

    if (std::strncmp(command.event_id, visitor_session_id_,
                     std::strlen(visitor_session_id_)) != 0) {
        return;
    }
    if (std::strcmp(type, "PTT_START") == 0) {
        command.type = IntercomCommandType::StartReply;
    } else if (std::strcmp(type, "PTT_CANCEL") == 0) {
        command.type = IntercomCommandType::CancelReply;
    } else if (std::strcmp(type, "PLAY_AUDIO") == 0 &&
               command.audio_url[0] != '\0') {
        command.type = IntercomCommandType::PlayAudio;
    } else {
        return;
    }

    if (xQueueSend(command_queue_, &command, 0) != pdTRUE) {
        ESP_LOGW(kTag, "Intercom command queue full");
    }
}

esp_err_t IntercomService::play_wav(const IntercomCommand &command,
                                    std::uint32_t max_duration_ms)
{
    if (!audio_bus_acquire(pdMS_TO_TICKS(1000))) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t result = ESP_FAIL;
    i2s_chan_handle_t tx_channel = nullptr;
    std::uint32_t data_length = 0;
    std::uint32_t duration_ms = 0;
    esp_http_client_config_t http_config = {};
    http_config.url = command.audio_url;
    http_config.method = HTTP_METHOD_GET;
    http_config.timeout_ms = static_cast<int>((std::min)(
        static_cast<std::uint32_t>(25000), max_duration_ms + 5000));
    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (!client) {
        audio_bus_release();
        return ESP_ERR_NO_MEM;
    }
#ifdef GATEWAY_API_KEY
    esp_http_client_set_header(client, "X-API-Key", GATEWAY_API_KEY);
#endif

    if (esp_http_client_open(client, 0) != ESP_OK ||
        esp_http_client_fetch_headers(client) < 0 ||
        esp_http_client_get_status_code(client) != 200) {
        goto cleanup;
    }

    std::uint8_t header[44];
    if (read_exact(client, header, sizeof(header)) != ESP_OK ||
        std::memcmp(header, "RIFF", 4) != 0 ||
        std::memcmp(header + 8, "WAVE", 4) != 0 ||
        std::memcmp(header + 36, "data", 4) != 0 ||
        read_u16(header + 20) != 1 || read_u16(header + 22) != 1 ||
        read_u32(header + 24) != kSampleRateHz || read_u16(header + 34) != 16) {
        ESP_LOGE(kTag, "Reply is not 16 kHz mono PCM WAV");
        goto cleanup;
    }
    data_length = read_u32(header + 40);
    duration_ms = data_length / 32;
    if ((data_length & 1U) != 0 || data_length > 640000 ||
        duration_ms == 0 || duration_ms > max_duration_ms) {
        ESP_LOGE(kTag, "Reply duration %u ms exceeds remaining session budget",
                 static_cast<unsigned>(duration_ms));
        goto cleanup;
    }

    {
        i2s_chan_config_t channel_config =
            I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
        if (i2s_new_channel(&channel_config, &tx_channel, nullptr) != ESP_OK) {
            goto cleanup;
        }
        i2s_std_config_t i2s_config = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(kSampleRateHz),
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
            .gpio_cfg = {
                .mclk = I2S_GPIO_UNUSED,
                .bclk = I2S_AUDIO_SCK,
                .ws = I2S_AUDIO_WS,
                .dout = I2S_SPK_SD,
                .din = I2S_GPIO_UNUSED,
                .invert_flags = {},
            },
        };
        i2s_config.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
        if (i2s_channel_init_std_mode(tx_channel, &i2s_config) != ESP_OK ||
            i2s_channel_enable(tx_channel) != ESP_OK) {
            goto cleanup;
        }
    }

    gpio_set_level(AMP_EN_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_LOGI(kTag,
             "Playing homeowner reply message=%s attenuation=-%u dB ramp=%u ms",
             command.message_id, speaker_output_attenuation_db(),
             speaker_output_ramp_ms());
    {
        std::uint8_t carry = 0;
        bool have_carry = false;
        std::uint32_t bytes_remaining = data_length;
        std::size_t frames_played = 0;
        speaker_envelope_t envelope{};
        speaker_envelope_init(&envelope, kSampleRateHz, data_length / 2U);
        while (bytes_remaining > 0) {
            const std::size_t requested = (std::min)(
                sizeof(playback_mono_),
                static_cast<std::size_t>(bytes_remaining));
            int read = esp_http_client_read(client,
                                            reinterpret_cast<char *>(playback_mono_),
                                            requested);
            if (read < 0) goto cleanup;
            if (read == 0) goto cleanup;
            bytes_remaining -= static_cast<std::uint32_t>(read);
            std::size_t byte_offset = 0;
            std::size_t frames = 0;
            if (have_carry) {
                const std::int16_t sample = static_cast<std::int16_t>(
                    carry |
                    (static_cast<std::uint16_t>(playback_mono_[0]) << 8));
                const std::int16_t scaled =
                    speaker_envelope_apply(&envelope, sample, frames_played++);
                playback_stereo_[0] = scaled;
                playback_stereo_[1] = scaled;
                frames = 1;
                byte_offset = 1;
                have_carry = false;
            }
            while (byte_offset + 1 < static_cast<std::size_t>(read)) {
                const std::int16_t sample = static_cast<std::int16_t>(
                    playback_mono_[byte_offset] |
                    (static_cast<std::uint16_t>(
                         playback_mono_[byte_offset + 1])
                     << 8));
                const std::int16_t scaled =
                    speaker_envelope_apply(&envelope, sample, frames_played++);
                playback_stereo_[frames * 2] = scaled;
                playback_stereo_[frames * 2 + 1] = scaled;
                ++frames;
                byte_offset += 2;
            }
            if (byte_offset < static_cast<std::size_t>(read)) {
                carry = playback_mono_[byte_offset];
                have_carry = true;
            }
            std::size_t bytes_written = 0;
            if (frames > 0 &&
                i2s_channel_write(tx_channel, playback_stereo_,
                                  frames * 2 * sizeof(std::int16_t),
                                  &bytes_written, pdMS_TO_TICKS(500)) != ESP_OK) {
                goto cleanup;
            }
        }
        constexpr std::size_t kSilenceSamples = 256 * 2;
        std::memset(playback_stereo_, 0,
                    kSilenceSamples * sizeof(playback_stereo_[0]));
        std::size_t silence_written = 0;
        (void)i2s_channel_write(tx_channel, playback_stereo_,
                                kSilenceSamples * sizeof(playback_stereo_[0]),
                                &silence_written, pdMS_TO_TICKS(100));
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    gpio_set_level(AMP_EN_PIN, 0);
    result = acknowledge(command.ack_url);
    if (result != ESP_OK) {
        ESP_LOGW(kTag, "Playback finished but delivery acknowledgement failed");
        result = ESP_OK;
    }

cleanup:
    gpio_set_level(AMP_EN_PIN, 0);
    if (tx_channel) {
        (void)i2s_channel_disable(tx_channel);
        (void)i2s_del_channel(tx_channel);
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    audio_bus_release();
    return result;
}

esp_err_t IntercomService::acknowledge(const char *url) const
{
    if (!url || url[0] == '\0') return ESP_OK;
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 5000;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_ERR_NO_MEM;
#ifdef GATEWAY_API_KEY
    esp_http_client_set_header(client, "X-API-Key", GATEWAY_API_KEY);
#endif
    const esp_err_t request_err = esp_http_client_perform(client);
    const int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    return request_err == ESP_OK && status >= 200 && status < 300
               ? ESP_OK : ESP_FAIL;
}

void IntercomService::stop()
{
    if (mqtt_client_) {
        (void)esp_mqtt_client_stop(mqtt_client_);
        (void)esp_mqtt_client_destroy(mqtt_client_);
        mqtt_client_ = nullptr;
    }
    if (command_queue_) {
        vQueueDelete(command_queue_);
        command_queue_ = nullptr;
    }
    mqtt_payload_size_ = 0;
    visitor_session_id_[0] = '\0';
    gpio_set_level(AMP_EN_PIN, 0);
}

}  // namespace doorbell
