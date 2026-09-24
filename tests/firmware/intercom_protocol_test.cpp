#include "services/intercom_protocol.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

void expect(bool condition, const char *description)
{
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        std::exit(1);
    }
}

void test_fragment_assembly()
{
    using doorbell::MqttFragmentResult;
    doorbell::MqttPayloadAssembly assembly;
    char payload[16]{};

    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 0, 5, "he", 2) ==
               MqttFragmentResult::InProgress,
           "first valid fragment is accepted");
    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 2, 5, "llo", 3) ==
               MqttFragmentResult::Complete,
           "contiguous valid fragment completes");
    expect(std::strcmp(payload, "hello") == 0,
           "completed payload is terminated");

    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 0, 15,
               "123456789012345", 15) == MqttFragmentResult::Complete,
           "capacity minus terminator is accepted");
    expect(payload[15] == '\0', "maximum payload is terminated in bounds");

    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 0, 16, "12345678", 8) ==
               MqttFragmentResult::Rejected,
           "payload with no terminator space is rejected");
    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 8, 16, "12345678", 8) ==
               MqttFragmentResult::Rejected,
           "continuation of rejected oversized payload stays rejected");

    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 0, 6, "abc", 3) ==
               MqttFragmentResult::InProgress,
           "gap test starts");
    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 4, 6, "ef", 2) ==
               MqttFragmentResult::Rejected,
           "fragment gap is rejected");
    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 3, 6, "def", 3) ==
               MqttFragmentResult::Rejected,
           "message remains inactive after malformed fragment");

    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 0, 6, "abc", 3) ==
               MqttFragmentResult::InProgress,
           "total mismatch test starts");
    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 3, 7, "def", 3) ==
               MqttFragmentResult::Rejected,
           "changing total length is rejected");

    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), -1, 2, "x", 1) ==
               MqttFragmentResult::Rejected,
           "negative offset is rejected");
    expect(doorbell::append_mqtt_payload_fragment(
               assembly, payload, sizeof(payload), 0, 2, nullptr, 1) ==
               MqttFragmentResult::Rejected,
           "null fragment is rejected");
}

void test_gateway_url_policy()
{
    using doorbell::GatewayCommandUrlKind;
    constexpr const char *gateway = "http://192.0.2.10:8080/api/events";
    constexpr const char *uuid = "123e4567-e89b-12d3-a456-426614174000";

    expect(doorbell::gateway_command_url_allowed(
               "http://192.0.2.10:8080/api/events/media/42-reply.wav",
               gateway, GatewayCommandUrlKind::Audio),
           "gateway audio route is accepted");
    expect(doorbell::gateway_command_url_allowed(
               "http://192.0.2.10:8080/api/system/ptt/messages/"
               "123e4567-e89b-12d3-a456-426614174000/delivered",
               gateway, GatewayCommandUrlKind::Acknowledgement),
           "gateway acknowledgement route is accepted");
    expect(doorbell::intercom_uuid_allowed(uuid), "canonical UUID is accepted");

    expect(!doorbell::gateway_command_url_allowed(
               "http://attacker.invalid/audio.wav", gateway,
               GatewayCommandUrlKind::Audio),
           "foreign origin is rejected");
    expect(!doorbell::gateway_command_url_allowed(
               "http://192.0.2.10:8080.attacker.invalid/api/events/media/a.wav",
               gateway, GatewayCommandUrlKind::Audio),
           "trusted-host prefix confusion is rejected");
    expect(!doorbell::gateway_command_url_allowed(
               "http://192.0.2.10:8080@attacker.invalid/api/events/media/a.wav",
               gateway, GatewayCommandUrlKind::Audio),
           "URL user-info confusion is rejected");
    expect(!doorbell::gateway_command_url_allowed(
               "http://192.0.2.10:8080/api/events/media/../admin",
               gateway, GatewayCommandUrlKind::Audio),
           "path traversal is rejected");
    expect(!doorbell::gateway_command_url_allowed(
               "http://192.0.2.10:8080/api/events/media/%2e%2e/admin",
               gateway, GatewayCommandUrlKind::Audio),
           "encoded path traversal is rejected");
    expect(!doorbell::gateway_command_url_allowed(
               "http://192.0.2.10:8080/api/events/media/a.wav#https://attacker.invalid",
               gateway, GatewayCommandUrlKind::Audio),
           "fragment is rejected");
    expect(!doorbell::gateway_command_url_allowed(
               "http://192.0.2.10:8080/api/events/media/a.wav?redirect=1",
               gateway, GatewayCommandUrlKind::Audio),
           "query string is rejected");
    expect(!doorbell::gateway_command_url_allowed(
               "http://192.0.2.10:8080/api/system/ptt/messages/not-a-uuid/delivered",
               gateway, GatewayCommandUrlKind::Acknowledgement),
           "malformed acknowledgement identifier is rejected");
    expect(!doorbell::intercom_uuid_allowed(
               "123e4567-e89b-12d3-a456-42661417400z"),
           "non-hex UUID is rejected");
}

}  // namespace

int main()
{
    test_fragment_assembly();
    test_gateway_url_policy();
    std::cout << "intercom protocol guard tests passed\n";
    return 0;
}
