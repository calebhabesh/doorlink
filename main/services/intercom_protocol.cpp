#include "services/intercom_protocol.hpp"

#include <cstring>

namespace doorbell {
namespace {

bool is_ascii_control_or_space(unsigned char value)
{
    return value <= 0x20U || value == 0x7fU;
}

bool is_hex(char value)
{
    return (value >= '0' && value <= '9') ||
           (value >= 'a' && value <= 'f') ||
           (value >= 'A' && value <= 'F');
}

bool is_media_key_character(char value)
{
    return (value >= 'a' && value <= 'z') ||
           (value >= 'A' && value <= 'Z') ||
           (value >= '0' && value <= '9') || value == '-' || value == '_' ||
           value == '.';
}

bool parse_http_origin(const char *url, std::size_t &origin_length)
{
    if (!url) return false;

    const char *authority = nullptr;
    if (std::strncmp(url, "http://", 7) == 0) {
        authority = url + 7;
    } else if (std::strncmp(url, "https://", 8) == 0) {
        authority = url + 8;
    } else {
        return false;
    }

    if (*authority == '\0') return false;
    const char *cursor = authority;
    while (*cursor != '\0' && *cursor != '/') {
        const auto value = static_cast<unsigned char>(*cursor);
        if (*cursor == '@' || *cursor == '\\' || *cursor == '?' ||
            *cursor == '#' || is_ascii_control_or_space(value)) {
            return false;
        }
        ++cursor;
    }
    if (cursor == authority) return false;
    origin_length = static_cast<std::size_t>(cursor - url);
    return true;
}

bool path_has_forbidden_character(const char *path)
{
    for (const char *cursor = path; *cursor != '\0'; ++cursor) {
        const auto value = static_cast<unsigned char>(*cursor);
        if (*cursor == '\\' || *cursor == '?' || *cursor == '#' ||
            *cursor == '%' || is_ascii_control_or_space(value)) {
            return true;
        }
    }
    return false;
}

bool media_path_allowed(const char *path)
{
    constexpr const char *kPrefix = "/api/events/media/";
    constexpr std::size_t kPrefixLength = 18;
    if (std::strncmp(path, kPrefix, kPrefixLength) != 0) return false;

    const char *key = path + kPrefixLength;
    if (*key == '\0' || std::strcmp(key, ".") == 0 ||
        std::strcmp(key, "..") == 0) {
        return false;
    }
    for (const char *cursor = key; *cursor != '\0'; ++cursor) {
        if (!is_media_key_character(*cursor)) return false;
    }
    return std::strstr(key, "..") == nullptr;
}

bool acknowledgement_path_allowed(const char *path)
{
    constexpr const char *kPrefix = "/api/system/ptt/messages/";
    constexpr std::size_t kPrefixLength = 25;
    constexpr const char *kSuffix = "/delivered";
    constexpr std::size_t kSuffixLength = 10;

    if (std::strncmp(path, kPrefix, kPrefixLength) != 0) return false;
    const char *message_id = path + kPrefixLength;
    const char *suffix = std::strstr(message_id, kSuffix);
    if (!suffix || std::strlen(suffix) != kSuffixLength ||
        static_cast<std::size_t>(suffix - message_id) != 36) {
        return false;
    }

    char uuid[37];
    std::memcpy(uuid, message_id, 36);
    uuid[36] = '\0';
    return intercom_uuid_allowed(uuid);
}

}  // namespace

void reset_mqtt_payload_assembly(MqttPayloadAssembly &assembly)
{
    assembly.received = 0;
    assembly.expected_total = 0;
    assembly.accepting = false;
}

MqttFragmentResult append_mqtt_payload_fragment(
    MqttPayloadAssembly &assembly, char *payload, std::size_t payload_capacity,
    int current_offset, int total_length, const char *fragment,
    int fragment_length)
{
    if (current_offset == 0) {
        reset_mqtt_payload_assembly(assembly);
        if (!payload || payload_capacity < 2 || !fragment ||
            total_length <= 0 || fragment_length <= 0 ||
            total_length >= static_cast<int>(payload_capacity) ||
            fragment_length > total_length) {
            return MqttFragmentResult::Rejected;
        }
        assembly.expected_total = total_length;
        assembly.accepting = true;
    } else if (!assembly.accepting) {
        return MqttFragmentResult::Rejected;
    }

    if (!fragment || current_offset < 0 || fragment_length <= 0 ||
        total_length != assembly.expected_total ||
        current_offset != static_cast<int>(assembly.received) ||
        current_offset > total_length || fragment_length > total_length ||
        current_offset > total_length - fragment_length) {
        reset_mqtt_payload_assembly(assembly);
        return MqttFragmentResult::Rejected;
    }

    const std::size_t offset = static_cast<std::size_t>(current_offset);
    const std::size_t length = static_cast<std::size_t>(fragment_length);
    if (offset >= payload_capacity || length >= payload_capacity - offset) {
        reset_mqtt_payload_assembly(assembly);
        return MqttFragmentResult::Rejected;
    }

    std::memcpy(payload + offset, fragment, length);
    assembly.received += length;
    if (assembly.received == static_cast<std::size_t>(assembly.expected_total)) {
        payload[assembly.received] = '\0';
        assembly.accepting = false;
        return MqttFragmentResult::Complete;
    }
    return MqttFragmentResult::InProgress;
}

bool intercom_uuid_allowed(const char *candidate)
{
    if (!candidate || std::strlen(candidate) != 36) return false;
    for (std::size_t index = 0; index < 36; ++index) {
        const bool hyphen_position =
            index == 8 || index == 13 || index == 18 || index == 23;
        if (hyphen_position ? candidate[index] != '-'
                            : !is_hex(candidate[index])) {
            return false;
        }
    }
    return true;
}

bool gateway_command_url_allowed(const char *candidate,
                                 const char *gateway_api_url,
                                 GatewayCommandUrlKind kind)
{
    std::size_t trusted_origin_length = 0;
    std::size_t candidate_origin_length = 0;
    if (!parse_http_origin(gateway_api_url, trusted_origin_length) ||
        !parse_http_origin(candidate, candidate_origin_length) ||
        candidate_origin_length != trusted_origin_length ||
        std::memcmp(candidate, gateway_api_url, trusted_origin_length) != 0 ||
        candidate[candidate_origin_length] != '/') {
        return false;
    }

    const char *path = candidate + candidate_origin_length;
    if (path_has_forbidden_character(path)) return false;
    return kind == GatewayCommandUrlKind::Audio
               ? media_path_allowed(path)
               : acknowledgement_path_allowed(path);
}

}  // namespace doorbell
