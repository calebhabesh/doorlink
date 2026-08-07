#include "doorbell_controller.hpp"

#include <cstdio>
#include <inttypes.h>

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "chime_player.h"
#include "ring_fade.h"
#include "wake_stub.h"

namespace doorbell {
namespace {
constexpr const char *kTag = "DoorbellController";
constexpr unsigned kCaptureAttempts = 2;
constexpr unsigned kTriggerAttempts = 2;
constexpr unsigned kUploadAttempts = 2;
constexpr std::uint32_t kRingFadeInMs = 1800;
constexpr std::uint32_t kRingHoldMs = 2500;
constexpr std::uint32_t kRingFadeOutMs = 1800;

#ifdef CONFIG_SMART_DOORBELL_ENABLE_PIR_EVENTS
constexpr bool kPirEventsEnabled = true;
#else
constexpr bool kPirEventsEnabled = false;
#endif

#ifndef DEVICE_ID
#define DEVICE_ID "smart-doorbell"
#endif

const char *state_name(DeviceState state)
{
    switch (state) {
    case DeviceState::Booting:
        return "BOOTING";
    case DeviceState::IdlePreparation:
        return "IDLE_PREPARATION";
    case DeviceState::Capturing:
        return "CAPTURING";
    case DeviceState::Connecting:
        return "CONNECTING";
    case DeviceState::Triggering:
        return "TRIGGERING";
    case DeviceState::Disconnecting:
        return "DISCONNECTING";
    case DeviceState::Uploading:
        return "UPLOADING";
    case DeviceState::ShuttingDown:
        return "SHUTTING_DOWN";
    }
    return "UNKNOWN";
}

void make_event_id(char (&event_id)[33])
{
    std::snprintf(event_id, sizeof(event_id),
                  "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32,
                  esp_random(), esp_random(), esp_random(), esp_random());
}

void log_elapsed(const char *stage, std::int64_t started_us)
{
    ESP_LOGI(kTag, "Latency %s: %.1f ms since controller start", stage,
             static_cast<double>(esp_timer_get_time() - started_us) / 1000.0);
}

const char *wake_name(WakeReason reason)
{
    switch (reason) {
    case WakeReason::ColdBoot:
        return "COLD_BOOT";
    case WakeReason::Button:
        return "BUTTON";
    case WakeReason::Motion:
        return "MOTION";
    case WakeReason::TimerRecovery:
        return "TIMER_RECOVERY";
    case WakeReason::Other:
        return "OTHER";
    }
    return "UNKNOWN";
}
}  // namespace

void DoorbellController::set_state(DeviceState state)
{
    state_ = state;
    ESP_LOGI(kTag, "State -> %s", state_name(state_));
}

esp_err_t DoorbellController::capture_with_retry(CapturedImage &image)
{
    esp_err_t err = ESP_FAIL;
    for (unsigned attempt = 1; attempt <= kCaptureAttempts; ++attempt) {
        ESP_LOGI(kTag, "Capture attempt %u/%u", attempt, kCaptureAttempts);
        err = camera_.capture(image);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        if (attempt < kCaptureAttempts) {
            vTaskDelay(pdMS_TO_TICKS(250));
        }
    }
    return err;
}

esp_err_t DoorbellController::upload_with_retry(const CapturedImage &image,
                                                const char *event_type,
                                                const char *event_id,
                                                const char *device_id,
                                                const char *firmware_version)
{
    esp_err_t err = ESP_FAIL;
    for (unsigned attempt = 1; attempt <= kUploadAttempts; ++attempt) {
        ESP_LOGI(kTag, "Upload attempt %u/%u", attempt, kUploadAttempts);
        err = connectivity_.upload(image, event_type, event_id, device_id,
                                   firmware_version);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        ESP_LOGW(kTag, "Upload attempt failed: %s", esp_err_to_name(err));
        if (attempt < kUploadAttempts) {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    return err;
}

esp_err_t DoorbellController::trigger_with_retry(
    const char *event_id, const char *event_type, const char *device_id,
    const char *firmware_version)
{
    esp_err_t err = ESP_FAIL;
    for (unsigned attempt = 1; attempt <= kTriggerAttempts; ++attempt) {
        ESP_LOGI(kTag, "Early trigger attempt %u/%u", attempt,
                 kTriggerAttempts);
        err = connectivity_.trigger(event_id, event_type, device_id,
                                    firmware_version);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        if (attempt < kTriggerAttempts) {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    return err;
}

[[noreturn]] void DoorbellController::sleep()
{
    set_state(DeviceState::ShuttingDown);
    (void)connectivity_.shutdown();
    ring_fade_stop();
    gpio_set_level(STATUS_LED_PIN, 0);
    power_.enter_deep_sleep(kPirEventsEnabled);
}

[[noreturn]] void DoorbellController::run()
{
    const std::int64_t started_us = esp_timer_get_time();
    wake_stub_install();

    const esp_err_t safe_err = power_.initialize_safe_state();
    if (safe_err != ESP_OK) {
        ESP_LOGE(kTag, "Safe-state initialization failed: %s",
                 esp_err_to_name(safe_err));
        sleep();
    }

    const WakeReason reason = power_.wake_reason();

    // D3 was asserted by the RTC wake stub before the bootloader. Begin the
    // button ring's gradual fade as soon as application GPIO is available.
    if (reason == WakeReason::Button) {
        gpio_set_level(STATUS_LED_PIN, 1);
        const esp_err_t fade_err = ring_animation_start(
            kRingFadeInMs, kRingHoldMs, kRingFadeOutMs);
        if (fade_err != ESP_OK) {
            ESP_LOGE(kTag, "GPIO48 fade-in failed: %s",
                     esp_err_to_name(fade_err));
        }
        const esp_err_t chime_err = chime_player_play_async();
        if (chime_err != ESP_OK) {
            ESP_LOGE(kTag, "Local chime playback failed: %s",
                     esp_err_to_name(chime_err));
        }
    } else if (reason == WakeReason::Motion) {
        gpio_set_level(STATUS_LED_PIN, 1);
    }

    set_state(DeviceState::Booting);
    ESP_LOGI(kTag, "Wake reason: %s", wake_name(reason));

    if (reason == WakeReason::Motion && !kPirEventsEnabled) {
        ESP_LOGW(kTag, "Ignoring PIR wake because production PIR events are disabled");
        set_state(DeviceState::IdlePreparation);
        sleep();
    }

    if (reason != WakeReason::Button && reason != WakeReason::Motion) {
        set_state(DeviceState::IdlePreparation);
        sleep();
    }

    const char *event_type = reason == WakeReason::Button
                                 ? "DOORBELL_PRESS"
                                 : "PIR_MOTION";
    const char *firmware_version = esp_app_get_description()->version;
    char event_id[33];
    make_event_id(event_id);
    ESP_LOGI(kTag, "Event id=%s device=%s firmware=%s", event_id, DEVICE_ID,
             firmware_version);

    // Alert first. Regardless of whether the response is received, the same
    // event ID is sent with the later upload so the gateway can decide whether
    // fallback notification is needed without creating duplicates.
    set_state(DeviceState::Connecting);
    esp_err_t err = connectivity_.connect();
    log_elapsed("early Wi-Fi connected/failed", started_us);
    if (err == ESP_OK) {
        set_state(DeviceState::Triggering);
        esp_err_t trigger_err = trigger_with_retry(
            event_id, event_type, DEVICE_ID, firmware_version);
        log_elapsed(trigger_err == ESP_OK ? "early trigger acknowledged"
                                          : "early trigger unconfirmed",
                    started_us);
        if (trigger_err != ESP_OK) {
            ESP_LOGW(kTag,
                     "Early trigger unconfirmed; upload will request fallback: %s",
                     esp_err_to_name(trigger_err));
        }
    } else {
        ESP_LOGW(kTag,
                 "Early Wi-Fi unavailable; continuing to capture for upload fallback: %s",
                 esp_err_to_name(err));
    }

    set_state(DeviceState::Disconnecting);
    esp_err_t shutdown_err = connectivity_.shutdown();
    log_elapsed("early Wi-Fi fully stopped", started_us);
    if (shutdown_err != ESP_OK) {
        ESP_LOGE(kTag, "Could not establish RF-off capture boundary: %s",
                 esp_err_to_name(shutdown_err));
        sleep();
    }

    CapturedImage image;
    set_state(DeviceState::Capturing);
    err = capture_with_retry(image);
    log_elapsed("QXGA capture complete/failed", started_us);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Event capture abandoned: %s", esp_err_to_name(err));
        sleep();
    }

    set_state(DeviceState::Connecting);
    err = connectivity_.connect();
    log_elapsed("upload Wi-Fi connected/failed", started_us);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Event upload abandoned: %s", esp_err_to_name(err));
        image.reset();
        sleep();
    }

    set_state(DeviceState::Uploading);
    err = upload_with_retry(image, event_type, event_id, DEVICE_ID,
                            firmware_version);
    log_elapsed("upload complete/failed", started_us);
    if (err == ESP_OK) {
        ESP_LOGI(kTag, "%s event completed (%ux%u, %u bytes)",
                 event_type,
                 static_cast<unsigned>(image.width()),
                 static_cast<unsigned>(image.height()),
                 static_cast<unsigned>(image.size()));
    } else {
        ESP_LOGE(kTag, "%s event failed: %s", event_type,
                 esp_err_to_name(err));
    }

    image.reset();
    sleep();
}

}  // namespace doorbell
