#include "services/intercom_service.hpp"

#include <algorithm>
#include <climits>
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
#include "esp_timer.h"
#include "freertos/task.h"
#include "psa/crypto.h"
#include "sdkconfig.h"

#if defined(CONFIG_SMART_DOORBELL_PRODUCTION_APP) && \
    CONFIG_SMART_DOORBELL_PRODUCTION_APP
#ifndef GATEWAY_API_KEY
#error "Production firmware requires GATEWAY_API_KEY for HTTP and MQTT command authentication"
#else
static_assert(sizeof(GATEWAY_API_KEY) > 1,
              "Production firmware requires a non-empty GATEWAY_API_KEY");
#endif
#endif

namespace doorbell {
namespace {
constexpr const char *kTag = "IntercomService";
#ifndef MQTT_PTT_TOPIC
#define MQTT_PTT_TOPIC "doorbell/commands/audio"
#endif
constexpr const char *kCommandTopic = MQTT_PTT_TOPIC;
constexpr std::uint32_t kSampleRateHz = 16000;
constexpr std::uint32_t kPlaybackOperationMaxMs = 25000;
constexpr std::uint32_t kHttpOperationSliceMaxMs = 5000;
constexpr std::uint32_t kPlaybackFinishReserveMs = 250;
constexpr std::uint32_t kI2sNoProgressMaxMs = 1500;

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

bool decode_sha256_hex(const char *hex, std::uint8_t (&decoded)[32])
{
    if (!hex || std::strlen(hex) != 64) return false;
    auto nibble = [](char value) -> int {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        return -1;
    };
    for (std::size_t index = 0; index < sizeof(decoded); ++index) {
        const int high = nibble(hex[index * 2]);
        const int low = nibble(hex[index * 2 + 1]);
        if (high < 0 || low < 0) return false;
        decoded[index] = static_cast<std::uint8_t>((high << 4) | low);
    }
    return true;
}

psa_status_t hmac_update_string(psa_mac_operation_t &operation,
                                const char *value)
{
    static constexpr std::uint8_t kSeparator = 0;
    if (!value) value = "";
    psa_status_t status = psa_mac_update(
        &operation, reinterpret_cast<const std::uint8_t *>(value),
        std::strlen(value));
    if (status == PSA_SUCCESS) {
        status = psa_mac_update(&operation, &kSeparator, 1);
    }
    return status;
}

bool intercom_signature_valid(const char *protocol, const char *type,
                              const IntercomCommand &command,
                              const char *signature)
{
#ifndef GATEWAY_API_KEY
    (void)protocol;
    (void)type;
    (void)command;
    (void)signature;
    return false;
#else
    std::uint8_t expected[32]{};
    if (!decode_sha256_hex(signature, expected)) return false;

    psa_status_t status = psa_crypto_init();
    psa_key_id_t key_id = 0;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    constexpr psa_algorithm_t kAlgorithm =
        PSA_ALG_HMAC(PSA_ALG_SHA_256);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attributes, kAlgorithm);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attributes, std::strlen(GATEWAY_API_KEY) * 8U);
    if (status == PSA_SUCCESS) {
        status = psa_import_key(
            &attributes,
            reinterpret_cast<const std::uint8_t *>(GATEWAY_API_KEY),
            std::strlen(GATEWAY_API_KEY), &key_id);
    }
    psa_reset_key_attributes(&attributes);

    psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
    if (status == PSA_SUCCESS) {
        status = psa_mac_sign_setup(&operation, key_id, kAlgorithm);
    }

    // Each ASCII value is NUL-delimited. Field order is protocol-defined and
    // independent of JSON object ordering or whitespace.
    constexpr const char *kDomain = "smart-doorbell-intercom-v1";
    const char *values[] = {
        kDomain, protocol, command.command_id, type, command.event_id,
        command.message_id, command.audio_url, command.ack_url,
    };
    for (const char *value : values) {
        if (status == PSA_SUCCESS) {
            status = hmac_update_string(operation, value);
        }
    }

    std::uint8_t computed[32]{};
    std::size_t computed_length = 0;
    if (status == PSA_SUCCESS) {
        status = psa_mac_sign_finish(&operation, computed, sizeof(computed),
                                     &computed_length);
    }
    (void)psa_mac_abort(&operation);
    if (key_id != 0) (void)psa_destroy_key(key_id);
    if (status != PSA_SUCCESS || computed_length != sizeof(computed)) {
        return false;
    }

    std::uint8_t difference = 0;
    for (std::size_t index = 0; index < sizeof(computed); ++index) {
        difference |= computed[index] ^ expected[index];
    }
    std::memset(computed, 0, sizeof(computed));
    std::memset(expected, 0, sizeof(expected));
    return difference == 0;
#endif
}

int deadline_timeout_ms(std::int64_t deadline_us,
                        std::uint32_t maximum_ms = kHttpOperationSliceMaxMs)
{
    const std::int64_t remaining_us = deadline_us - esp_timer_get_time();
    if (remaining_us <= 0) return 0;
    const std::int64_t rounded_ms = (remaining_us + 999LL) / 1000LL;
    const std::uint32_t capped_maximum_ms =
        (std::min)(maximum_ms, static_cast<std::uint32_t>(INT_MAX));
    return static_cast<int>((std::min)(
        rounded_ms, static_cast<std::int64_t>(capped_maximum_ms)));
}

esp_err_t apply_http_deadline(esp_http_client_handle_t client,
                              std::int64_t deadline_us,
                              std::uint32_t maximum_ms =
                                  kHttpOperationSliceMaxMs)
{
    const int timeout_ms = deadline_timeout_ms(deadline_us, maximum_ms);
    return timeout_ms > 0
               ? esp_http_client_set_timeout_ms(client, timeout_ms)
               : ESP_ERR_TIMEOUT;
}

esp_err_t read_exact(esp_http_client_handle_t client, std::uint8_t *data,
                     std::size_t length, std::int64_t deadline_us)
{
    std::size_t offset = 0;
    while (offset < length) {
        const esp_err_t timeout_err = apply_http_deadline(client, deadline_us);
        if (timeout_err != ESP_OK) return timeout_err;
        const int read = esp_http_client_read(
            client, reinterpret_cast<char *>(data + offset), length - offset);
        if (read <= 0) {
            return ESP_FAIL;
        }
        offset += static_cast<std::size_t>(read);
    }
    return ESP_OK;
}

esp_err_t write_i2s_bounded(i2s_chan_handle_t channel, const void *data,
                            std::size_t length, std::int64_t deadline_us)
{
    std::size_t offset = 0;
    std::int64_t last_progress_us = esp_timer_get_time();
    while (offset < length) {
        const int remaining_ms = deadline_timeout_ms(deadline_us, 500);
        if (remaining_ms <= 0) return ESP_ERR_TIMEOUT;

        std::size_t bytes_written = 0;
        const esp_err_t err = i2s_channel_write(
            channel, static_cast<const std::uint8_t *>(data) + offset,
            length - offset, &bytes_written,
            (std::max)(static_cast<TickType_t>(1),
                       pdMS_TO_TICKS(remaining_ms)));
        if (bytes_written > length - offset) return ESP_FAIL;
        offset += bytes_written;
        if (bytes_written > 0) last_progress_us = esp_timer_get_time();
        if (offset == length) return ESP_OK;
        if (err != ESP_OK && err != ESP_ERR_TIMEOUT) return err;
        if (err == ESP_OK && bytes_written == 0) return ESP_FAIL;
        if (bytes_written == 0 &&
            esp_timer_get_time() - last_progress_us >=
                static_cast<std::int64_t>(kI2sNoProgressMaxMs) * 1000LL) {
            return ESP_ERR_TIMEOUT;
        }
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
#if defined(MQTT_USERNAME) && defined(MQTT_PASSWORD)
    config.credentials.username = MQTT_USERNAME;
    config.credentials.authentication.password = MQTT_PASSWORD;
#elif defined(MQTT_USERNAME) || defined(MQTT_PASSWORD)
#error "Define both MQTT_USERNAME and MQTT_PASSWORD, or define neither"
#endif
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
        reset_mqtt_payload_assembly(mqtt_payload_assembly_);
        esp_mqtt_client_subscribe(event->client, kCommandTopic, 1);
        ESP_LOGI(kTag, "Subscribed to %s", kCommandTopic);
        return;
    }
    if (event_id == MQTT_EVENT_DISCONNECTED) {
        reset_mqtt_payload_assembly(mqtt_payload_assembly_);
        return;
    }
    if (event_id != MQTT_EVENT_DATA || !event) {
        return;
    }

    if (event->current_data_offset == 0 &&
        !topic_equals(event, kCommandTopic)) {
        reset_mqtt_payload_assembly(mqtt_payload_assembly_);
        return;
    }

    const bool was_accepting = mqtt_payload_assembly_.accepting;
    const MqttFragmentResult fragment_result = append_mqtt_payload_fragment(
        mqtt_payload_assembly_, mqtt_payload_, sizeof(mqtt_payload_),
        event->current_data_offset, event->total_data_len, event->data,
        event->data_len);
    if (fragment_result == MqttFragmentResult::Rejected) {
        if (event->current_data_offset == 0 &&
            event->total_data_len >= static_cast<int>(sizeof(mqtt_payload_))) {
            ESP_LOGW(kTag, "Discarding oversized intercom command (%d bytes)",
                     event->total_data_len);
        } else if (event->current_data_offset == 0 || was_accepting) {
            ESP_LOGW(kTag, "Discarding malformed MQTT command fragments");
        }
        return;
    }
    if (fragment_result == MqttFragmentResult::Complete) {
        parse_command(mqtt_payload_);
    }
}

void IntercomService::parse_command(const char *json)
{
    IntercomCommand command{};
    char type[24]{};
    char protocol[8]{};
    char signature[65]{};
    copy_json_string(json, "protocol", protocol, sizeof(protocol));
    copy_json_string(json, "commandId", command.command_id,
                     sizeof(command.command_id));
    copy_json_string(json, "signature", signature, sizeof(signature));
    copy_json_string(json, "type", type, sizeof(type));
    copy_json_string(json, "eventId", command.event_id,
                     sizeof(command.event_id));
    copy_json_string(json, "messageId", command.message_id,
                     sizeof(command.message_id));
    copy_json_string(json, "audioUrl", command.audio_url,
                     sizeof(command.audio_url));
    copy_json_string(json, "ackUrl", command.ack_url,
                     sizeof(command.ack_url));

    if (std::strcmp(protocol, "v1") != 0 ||
        !intercom_uuid_allowed(command.command_id) ||
        std::strcmp(command.event_id, visitor_session_id_) != 0 ||
        !intercom_signature_valid(protocol, type, command, signature)) {
        ESP_LOGW(kTag, "Discarding unauthenticated intercom command");
        return;
    }
    for (const auto &recent_command_id : recent_command_ids_) {
        if (std::strcmp(command.command_id, recent_command_id) == 0) {
            ESP_LOGI(kTag, "Ignoring replayed intercom command");
            return;
        }
    }
    if (std::strcmp(type, "PTT_START") == 0) {
        command.type = IntercomCommandType::StartReply;
    } else if (std::strcmp(type, "PTT_CANCEL") == 0) {
        command.type = IntercomCommandType::CancelReply;
    } else if (std::strcmp(type, "PLAY_AUDIO") == 0 &&
               intercom_uuid_allowed(command.message_id) &&
               gateway_command_url_allowed(command.audio_url,
                                           GATEWAY_API_URL,
                                           GatewayCommandUrlKind::Audio) &&
               gateway_command_url_allowed(
                   command.ack_url, GATEWAY_API_URL,
                   GatewayCommandUrlKind::Acknowledgement)) {
        if (std::strcmp(command.message_id, last_message_id_) == 0) {
            ESP_LOGI(kTag, "Ignoring duplicate homeowner reply command");
            return;
        }
        command.type = IntercomCommandType::PlayAudio;
    } else {
        ESP_LOGW(kTag, "Discarding invalid intercom command");
        return;
    }

    if (xQueueSend(command_queue_, &command, 0) != pdTRUE) {
        ESP_LOGW(kTag, "Intercom command queue full");
    } else {
        std::snprintf(recent_command_ids_[next_recent_command_id_],
                      sizeof(recent_command_ids_[next_recent_command_id_]),
                      "%s", command.command_id);
        next_recent_command_id_ =
            (next_recent_command_id_ + 1) % kRecentCommandCount;
        if (command.type == IntercomCommandType::PlayAudio) {
            std::snprintf(last_message_id_, sizeof(last_message_id_), "%s",
                          command.message_id);
        }
    }
}

esp_err_t IntercomService::play_wav(const IntercomCommand &command,
                                    std::uint32_t max_duration_ms)
{
    if (max_duration_ms == 0 ||
        !gateway_command_url_allowed(command.audio_url, GATEWAY_API_URL,
                                     GatewayCommandUrlKind::Audio) ||
        !gateway_command_url_allowed(command.ack_url, GATEWAY_API_URL,
                                     GatewayCommandUrlKind::Acknowledgement)) {
        return ESP_ERR_INVALID_ARG;
    }
    const std::uint32_t operation_budget_ms =
        (std::min)(max_duration_ms, kPlaybackOperationMaxMs);
    const std::int64_t deadline_us =
        esp_timer_get_time() +
        static_cast<std::int64_t>(operation_budget_ms) * 1000LL;

    if (!audio_bus_acquire(pdMS_TO_TICKS(1000))) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t result = ESP_FAIL;
    i2s_chan_handle_t tx_channel = nullptr;
    std::uint32_t data_length = 0;
    std::uint32_t duration_ms = 0;
    int time_after_header_ms = 0;
    esp_http_client_config_t http_config = {};
    http_config.url = command.audio_url;
    http_config.method = HTTP_METHOD_GET;
    http_config.timeout_ms = deadline_timeout_ms(deadline_us);
    http_config.disable_auto_redirect = true;
    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (!client) {
        audio_bus_release();
        return ESP_ERR_NO_MEM;
    }
#ifdef GATEWAY_API_KEY
    esp_http_client_set_header(client, "X-API-Key", GATEWAY_API_KEY);
#endif

    if (apply_http_deadline(client, deadline_us) != ESP_OK ||
        esp_http_client_open(client, 0) != ESP_OK ||
        apply_http_deadline(client, deadline_us) != ESP_OK ||
        esp_http_client_fetch_headers(client) < 0 ||
        esp_http_client_get_status_code(client) != 200) {
        goto cleanup;
    }

    std::uint8_t header[44];
    if (read_exact(client, header, sizeof(header), deadline_us) != ESP_OK ||
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
    time_after_header_ms = deadline_timeout_ms(
        deadline_us, kPlaybackOperationMaxMs);
    if ((data_length & 1U) != 0 || data_length > 640000 ||
        duration_ms == 0 || duration_ms > max_duration_ms ||
        time_after_header_ms <= 0 ||
        duration_ms + kPlaybackFinishReserveMs >
            static_cast<std::uint32_t>(time_after_header_ms)) {
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
            if (apply_http_deadline(client, deadline_us, 1000) != ESP_OK) {
                result = ESP_ERR_TIMEOUT;
                goto cleanup;
            }
            const std::size_t requested = (std::min)(
                sizeof(playback_mono_),
                static_cast<std::size_t>(bytes_remaining));
            int read = esp_http_client_read(client,
                                            reinterpret_cast<char *>(playback_mono_),
                                            requested);
            if (read <= 0) {
                result = ESP_FAIL;
                goto cleanup;
            }
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
            if (frames > 0) {
                result = write_i2s_bounded(
                    tx_channel, playback_stereo_,
                    frames * 2 * sizeof(std::int16_t), deadline_us);
                if (result != ESP_OK) goto cleanup;
            }
        }
        if (have_carry) {
            result = ESP_FAIL;
            goto cleanup;
        }
        constexpr std::size_t kSilenceSamples = 256 * 2;
        std::memset(playback_stereo_, 0,
                    kSilenceSamples * sizeof(playback_stereo_[0]));
        (void)write_i2s_bounded(
            tx_channel, playback_stereo_,
            kSilenceSamples * sizeof(playback_stereo_[0]), deadline_us);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    gpio_set_level(AMP_EN_PIN, 0);
    result = acknowledge(command.ack_url, deadline_us);
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

esp_err_t IntercomService::acknowledge(const char *url,
                                       std::int64_t deadline_us) const
{
    if (!url || url[0] == '\0') return ESP_OK;
    if (!gateway_command_url_allowed(url, GATEWAY_API_URL,
                                     GatewayCommandUrlKind::Acknowledgement)) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = deadline_timeout_ms(deadline_us);
    config.disable_auto_redirect = true;
    if (config.timeout_ms <= 0) return ESP_ERR_TIMEOUT;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_ERR_NO_MEM;
#ifdef GATEWAY_API_KEY
    esp_http_client_set_header(client, "X-API-Key", GATEWAY_API_KEY);
#endif
    esp_err_t request_err = apply_http_deadline(client, deadline_us);
    if (request_err == ESP_OK) {
        request_err = esp_http_client_open(client, 0);
    }
    if (request_err == ESP_OK) {
        request_err = apply_http_deadline(client, deadline_us);
    }
    int status = 0;
    if (request_err == ESP_OK && esp_http_client_fetch_headers(client) >= 0) {
        status = esp_http_client_get_status_code(client);
    } else if (request_err == ESP_OK) {
        request_err = ESP_FAIL;
    }
    esp_http_client_close(client);
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
    reset_mqtt_payload_assembly(mqtt_payload_assembly_);
    visitor_session_id_[0] = '\0';
    last_message_id_[0] = '\0';
    std::memset(recent_command_ids_, 0, sizeof(recent_command_ids_));
    next_recent_command_id_ = 0;
    // play_wav() mutes AMP_EN in its own cleanup path while it owns the audio
    // bus. stop() only tears down the MQTT control plane and must not mute an
    // independently running local doorbell chime.
}

}  // namespace doorbell
