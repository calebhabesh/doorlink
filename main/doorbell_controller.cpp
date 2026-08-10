#include "doorbell_controller.hpp"

#include <cstdio>
#include <cstring>
#include <inttypes.h>

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
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
constexpr std::uint32_t kVisitorGreetingMs = 5000;
constexpr std::uint32_t kChimeReleaseWaitMs = 3500;

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
    case DeviceState::EarlyNotify:
        return "EARLY_NOTIFY";
    case DeviceState::RfQuiesce:
        return "RF_QUIESCE";
    case DeviceState::CameraPowerUp:
        return "CAMERA_POWER_UP";
    case DeviceState::Capturing:
        return "CAPTURING";
    case DeviceState::CameraPowerDown:
        return "CAMERA_POWER_DOWN";
    case DeviceState::RecordingGreeting:
        return "RECORDING_GREETING";
    case DeviceState::WifiReconnect:
        return "WIFI_RECONNECT";
    case DeviceState::Uploading:
        return "UPLOADING";
    case DeviceState::ListeningForReply:
        return "LISTENING_FOR_REPLY";
    case DeviceState::ReplyArmed:
        return "REPLY_ARMED";
    case DeviceState::PlayingReply:
        return "PLAYING_REPLY";
    case DeviceState::PreparingSleep:
        return "PREPARING_SLEEP";
    case DeviceState::WaitingForRelease:
        return "WAITING_FOR_RELEASE";
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

struct GreetingCaptureTaskContext {
    AudioService *audio_service;
    RecordedAudio *greeting;
    SemaphoreHandle_t completion;
    std::uint32_t duration_ms;
    std::int64_t session_start_us;
    esp_err_t result{ESP_FAIL};
};

void record_greeting_task(void *arg)
{
    auto *context = static_cast<GreetingCaptureTaskContext *>(arg);

    // The local chime and microphone share the I2S clocks. Camera capture does
    // not, so wait here while the controller starts the camera in parallel.
    for (std::uint32_t wait_ms = 0;
         chime_player_is_playing() && wait_ms < kChimeReleaseWaitMs;
         wait_ms += 20) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    ESP_LOGI(kTag,
             "[MONOTONIC_TIMING] t=%.1f ms | STAGE_START: Visitor microphone recording",
             static_cast<double>(esp_timer_get_time() - context->session_start_us) /
                 1000.0);
    context->result = context->audio_service->record_greeting(
        *context->greeting, context->duration_ms);
    xSemaphoreGive(context->completion);
    vTaskDelete(nullptr);
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

DoorbellController::DoorbellController()
{
    event_queue_ = xQueueCreate(10, sizeof(SystemEvent));
    button_mailbox_ = xQueueCreate(1, sizeof(SystemEvent));
    if (!event_queue_ || !button_mailbox_) {
        ESP_LOGE(kTag, "Failed to create system event queues");
    }
}

DoorbellController::~DoorbellController()
{
    if (event_queue_) {
        vQueueDelete(event_queue_);
        event_queue_ = nullptr;
    }
    if (button_mailbox_) {
        vQueueDelete(button_mailbox_);
        button_mailbox_ = nullptr;
    }
}

esp_err_t DoorbellController::post_button_event(const SystemEvent &event)
{
    if (button_mailbox_) {
        (void)xQueueOverwrite(button_mailbox_, &event);
    }
    return ESP_OK;
}

esp_err_t DoorbellController::post_event(const SystemEvent &event)
{
    if (!event_queue_) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xQueueSend(event_queue_, &event, pdMS_TO_TICKS(10)) != pdTRUE) {
        ESP_LOGW(kTag, "Event queue full, dropped event type %d",
                 static_cast<int>(event.type));
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

void DoorbellController::set_state(DeviceState state)
{
    state_ = state;
    wifi_rf_active_ = (state == DeviceState::EarlyNotify ||
                       state == DeviceState::WifiReconnect ||
                       state == DeviceState::Uploading ||
                       state == DeviceState::ListeningForReply ||
                       state == DeviceState::ReplyArmed ||
                       state == DeviceState::PlayingReply);
    cam_pwr_active_ = (state == DeviceState::CameraPowerUp ||
                       state == DeviceState::Capturing);

    if (!validate_invariant_pwr_01(wifi_rf_active_, cam_pwr_active_)) {
        ESP_LOGE(kTag, "CRITICAL INVARIANT VIOLATION: PWR-01 violated! (RF=%d, CAM=%d)",
                 wifi_rf_active_, cam_pwr_active_);
    }

    ESP_LOGI(kTag, "State -> %s (RF=%d, CAM=%d)", state_name(state_),
             wifi_rf_active_, cam_pwr_active_);
}

esp_err_t DoorbellController::capture_with_retry(CapturedImage &image, int timeout_ms)
{
    esp_err_t err = ESP_FAIL;
    for (unsigned attempt = 1; attempt <= kCaptureAttempts; ++attempt) {
        ESP_LOGI(kTag, "Capture attempt %u/%u (timeout=%d ms)", attempt, kCaptureAttempts, timeout_ms);
        err = camera_.capture(image, timeout_ms);
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
                                                const RecordedAudio *audio,
                                                const char *event_type,
                                                const char *event_id,
                                                const char *device_id,
                                                const char *firmware_version,
                                                int timeout_ms)
{
    esp_err_t err = ESP_FAIL;
    for (unsigned attempt = 1; attempt <= kUploadAttempts; ++attempt) {
        ESP_LOGI(kTag, "Upload attempt %u/%u (timeout=%d ms)", attempt, kUploadAttempts, timeout_ms);
        err = connectivity_.upload(image, audio, event_type, event_id, device_id,
                                   firmware_version, timeout_ms);
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
    const char *firmware_version, int timeout_ms)
{
    esp_err_t err = ESP_FAIL;
    for (unsigned attempt = 1; attempt <= kTriggerAttempts; ++attempt) {
        ESP_LOGI(kTag, "Early trigger attempt %u/%u (timeout=%d ms)", attempt,
                 kTriggerAttempts, timeout_ms);
        err = connectivity_.trigger(event_id, event_type, device_id,
                                    firmware_version, timeout_ms);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        if (attempt < kTriggerAttempts) {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    return err;
}

esp_err_t DoorbellController::handle_early_notify(const char *event_id, const char *event_type, const char *firmware_version, int remaining_ms)
{
    set_state(DeviceState::EarlyNotify);
    int wifi_timeout = (std::min)(10000, remaining_ms);
    esp_err_t err = connectivity_.connect(wifi_timeout);
    if (err == ESP_OK) {
        int trigger_timeout = (std::min)(5000, remaining_ms);
        esp_err_t trigger_err = trigger_with_retry(
            event_id, event_type, DEVICE_ID, firmware_version, trigger_timeout);
        if (trigger_err == ESP_OK) {
            session_.early_notified = true;
        } else {
            ESP_LOGW(kTag, "Early trigger unconfirmed: %s", esp_err_to_name(trigger_err));
        }
    } else {
        ESP_LOGW(kTag, "Early Wi-Fi unavailable: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t DoorbellController::handle_rf_quiesce()
{
    set_state(DeviceState::RfQuiesce);
    esp_err_t shutdown_err = connectivity_.shutdown();
    if (shutdown_err != ESP_OK) {
        ESP_LOGE(kTag, "Could not establish RF-off capture boundary: %s",
                 esp_err_to_name(shutdown_err));
    }
    return shutdown_err;
}

esp_err_t DoorbellController::handle_camera_capture(CapturedImage &image, int remaining_ms)
{
    set_state(DeviceState::CameraPowerUp);

    set_state(DeviceState::Capturing);
    int capture_timeout = (std::min)(4000, remaining_ms);
    esp_err_t err = capture_with_retry(image, capture_timeout);
    if (err == ESP_OK) {
        session_.image_captured = true;
    } else {
        ESP_LOGE(kTag, "Event capture abandoned: %s", esp_err_to_name(err));
    }

    set_state(DeviceState::CameraPowerDown);
    return err;
}

esp_err_t DoorbellController::handle_upload(const CapturedImage &image,
                                             const RecordedAudio *audio,
                                             const char *event_id,
                                             const char *event_type,
                                             const char *firmware_version,
                                             int remaining_ms)
{
    set_state(DeviceState::WifiReconnect);
    int wifi_timeout = (std::min)(10000, remaining_ms);
    esp_err_t err = connectivity_.connect(wifi_timeout);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Event upload abandoned: %s", esp_err_to_name(err));
        return err;
    }

    set_state(DeviceState::Uploading);
    int upload_timeout = (std::min)(30000, remaining_ms);
    err = upload_with_retry(image, audio, event_type, event_id, DEVICE_ID,
                            firmware_version, upload_timeout);
    if (err == ESP_OK) {
        session_.image_uploaded = true;
        ESP_LOGI(kTag, "%s event completed (%ux%u, %u bytes, event_id=%s)",
                 event_type,
                 static_cast<unsigned>(image.width()),
                 static_cast<unsigned>(image.height()),
                 static_cast<unsigned>(image.size()),
                 event_id);
    } else {
        ESP_LOGE(kTag, "%s event failed: %s", event_type, esp_err_to_name(err));
    }
    return err;
}

void DoorbellController::start_button_monitor()
{
    if (button_monitor_task_ != nullptr) {
        return;
    }
    button_monitor_running_ = true;
    xTaskCreate([](void *arg) {
        auto *self = static_cast<DoorbellController *>(arg);

        gpio_reset_pin(DOORBELL_BUTTON_PIN);
        gpio_set_direction(DOORBELL_BUTTON_PIN, GPIO_MODE_INPUT);
        gpio_pullup_en(DOORBELL_BUTTON_PIN);

        int initial_level = gpio_get_level(DOORBELL_BUTTON_PIN);
        int stable_btn_level = initial_level;
        int candidate_btn_level = initial_level;
        int64_t candidate_start_us = esp_timer_get_time();
        bool button_released = (initial_level == 1);

        while (self->button_monitor_running_) {
            int current_raw = gpio_get_level(DOORBELL_BUTTON_PIN);
            int64_t now_us = esp_timer_get_time();

            if (current_raw != candidate_btn_level) {
                candidate_btn_level = current_raw;
                candidate_start_us = now_us;
            } else if ((now_us - candidate_start_us) >= 10000LL &&
                       candidate_btn_level != stable_btn_level) {
                stable_btn_level = candidate_btn_level;
                if (stable_btn_level == 1) {
                    button_released = true;
                } else if (stable_btn_level == 0 && button_released) {
                    button_released = false;
                    ESP_LOGI(kTag, "⚡ Button repress detected by ButtonMonitor!");

                    // 1. Immediate local chime audio & ring LED feedback
                    (void)chime_player_play_async();

                    // 2. Post SystemEvent to dedicated length-1 button mailbox (xQueueOverwrite)
                    SystemEvent ev{EventType::ButtonPress, static_cast<uint32_t>(now_us / 1000LL)};
                    self->post_button_event(ev);
                }
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }

        self->button_monitor_task_ = nullptr;
        vTaskDelete(NULL);
    }, "button_monitor", 3072, this, configMAX_PRIORITIES - 3, &button_monitor_task_);
}

void DoorbellController::stop_button_monitor()
{
    if (button_monitor_task_ != nullptr) {
        button_monitor_running_ = false;
        while (button_monitor_task_ != nullptr) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void DoorbellController::execute_alert_cycle(const char *event_type, const char *firmware_version)
{
    const int64_t now_start_us = esp_timer_get_time();
    int remaining_ms = static_cast<int>(session_.remaining_us(now_start_us) / 1000LL);
    if (session_.is_expired(now_start_us) || remaining_ms <= 0) {
        ESP_LOGW(kTag, "session=%s cycle=%" PRIu32 " ALERT_SKIPPED reason=session_deadline_reached",
                 session_.session_id, session_.alert_cycle_count + 1);
        session_.followup_pending = false;
        return;
    }

    session_.alert_cycle_count++;
    session_.last_remote_alert_start_us = now_start_us;
    session_.followup_pending = false;
    alert_cycle_in_progress_ = true;
    alert_cycles_started_++;

    char cycle_event_id[40];
    std::snprintf(cycle_event_id, sizeof(cycle_event_id), "%s-%" PRIu32,
                  session_.session_id, session_.alert_cycle_count);

    ESP_LOGI(kTag, "[MONOTONIC_TIMING] t=%.1f ms | ALERT_START session=%s event_id=%s cycle=%" PRIu32 " type=%s remaining_time=%.1fs",
             static_cast<double>(now_start_us - session_.session_start_us) / 1000.0,
             session_.session_id, cycle_event_id, session_.alert_cycle_count, event_type,
             static_cast<double>(remaining_ms) / 1000.0);

    // 1. Instant Early Notification (bounded by remaining_ms)
    (void)handle_early_notify(cycle_event_id, event_type, firmware_version, remaining_ms);
    ESP_LOGI(kTag, "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: Early Notification (Wi-Fi connected & HTTP trigger sent)",
             static_cast<double>(esp_timer_get_time() - session_.session_start_us) / 1000.0);

    int64_t now_us = esp_timer_get_time();
    remaining_ms = static_cast<int>(session_.remaining_us(now_us) / 1000LL);
    if (remaining_ms <= 0) {
        ESP_LOGW(kTag, "session=%s cycle=%" PRIu32 " ALERT_ABORTED stage=early_notify reason=session_deadline_reached",
                 session_.session_id, session_.alert_cycle_count);
        session_.last_remote_alert_end_us = esp_timer_get_time();
        alert_cycle_in_progress_ = false;
        alert_cycles_finished_++;
        return;
    }

    // 2. RF Quiescence (PWR-01 Boundary: Ensure Wi-Fi RF is off before camera power-up)
    const esp_err_t shutdown_err = handle_rf_quiesce();
    if (shutdown_err != ESP_OK) {
        ESP_LOGE(kTag, "session=%s cycle=%" PRIu32 " ALERT_FAILED reason=rf_quiesce_failed",
                 session_.session_id, session_.alert_cycle_count);
        session_.last_remote_alert_end_us = esp_timer_get_time();
        alert_cycle_in_progress_ = false;
        alert_cycles_finished_++;
        return;
    }
    ESP_LOGI(kTag, "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: RF Quiesced (Wi-Fi disabled for camera capture)",
             static_cast<double>(esp_timer_get_time() - session_.session_start_us) / 1000.0);

    RecordedAudio greeting;
    const bool doorbell_event = std::strncmp(event_type, "DOORBELL_", 9) == 0;
    const int greeting_budget_ms = static_cast<int>((std::min)(
        static_cast<int64_t>(kVisitorGreetingMs),
        (std::max)(0LL, session_.remaining_us(esp_timer_get_time()) / 1000LL -
                            10000LL)));

    StaticSemaphore_t greeting_completion_storage{};
    SemaphoreHandle_t greeting_completion = nullptr;
    GreetingCaptureTaskContext greeting_context{};
    bool greeting_task_started = false;
    if (doorbell_event && greeting_budget_ms >= 500) {
        greeting_completion =
            xSemaphoreCreateBinaryStatic(&greeting_completion_storage);
        greeting_context = {
            .audio_service = &audio_,
            .greeting = &greeting,
            .completion = greeting_completion,
            .duration_ms = static_cast<std::uint32_t>(greeting_budget_ms),
            .session_start_us = session_.session_start_us,
            .result = ESP_FAIL,
        };
        greeting_task_started =
            xTaskCreate(record_greeting_task, "visitor_audio", 4096,
                        &greeting_context, configMAX_PRIORITIES - 4,
                        nullptr) == pdPASS;
        if (!greeting_task_started) {
            ESP_LOGW(kTag,
                     "Could not start parallel visitor recording; using sequential fallback");
        }
    }

    // 3. Capture the photo while the visitor greeting starts as soon as the
    // local chime releases the shared I2S bus.
    CapturedImage image;
    const esp_err_t capture_err = handle_camera_capture(image, remaining_ms);

    if (greeting_task_started) {
        // record_greeting() is bounded by its duration and I2S read timeouts.
        // Joining also keeps the stack-owned context alive until the task exits.
        xSemaphoreTake(greeting_completion, portMAX_DELAY);
        ESP_LOGI(kTag,
                 "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: Visitor microphone recording",
                 static_cast<double>(esp_timer_get_time() -
                                     session_.session_start_us) /
                     1000.0);
        if (greeting_context.result != ESP_OK) {
            ESP_LOGW(kTag, "Visitor greeting unavailable; continuing image-only: %s",
                     esp_err_to_name(greeting_context.result));
        }
    }

    if (capture_err != ESP_OK || !image.valid()) {
        ESP_LOGE(kTag, "session=%s cycle=%" PRIu32 " ALERT_FAILED reason=camera_capture_failed",
                 session_.session_id, session_.alert_cycle_count);
        image.reset();
        greeting.reset();
        session_.last_remote_alert_end_us = esp_timer_get_time();
        alert_cycle_in_progress_ = false;
        alert_cycles_finished_++;
        return;
    }
    ESP_LOGI(kTag, "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: Camera Photo Captured & Copied to PSRAM (SHUTTER CLOSE)",
             static_cast<double>(esp_timer_get_time() - session_.session_start_us) / 1000.0);

    if (!greeting_task_started && doorbell_event && greeting_budget_ms >= 500) {
        set_state(DeviceState::RecordingGreeting);
        const esp_err_t audio_err = audio_.record_greeting(
            greeting, static_cast<std::uint32_t>(greeting_budget_ms));
        if (audio_err != ESP_OK) {
            ESP_LOGW(kTag, "Visitor greeting unavailable; continuing image-only: %s",
                     esp_err_to_name(audio_err));
        }
    }

    // TIME-MAX check prior to Wi-Fi Reconnect & Upload
    now_us = esp_timer_get_time();
    remaining_ms = static_cast<int>(session_.remaining_us(now_us) / 1000LL);
    if (remaining_ms <= 0) {
        ESP_LOGW(kTag, "session=%s cycle=%" PRIu32 " ALERT_ABORTED stage=camera_capture reason=session_deadline_reached",
                 session_.session_id, session_.alert_cycle_count);
        image.reset();
        session_.last_remote_alert_end_us = esp_timer_get_time();
        alert_cycle_in_progress_ = false;
        alert_cycles_finished_++;
        return;
    }

    // 4. Wi-Fi Reconnect & Multipart Upload (bounded by remaining_ms)
    const esp_err_t upload_err = handle_upload(
        image, greeting.valid() ? &greeting : nullptr, cycle_event_id,
        event_type, firmware_version, remaining_ms);
    image.reset();
    greeting.reset();
    ESP_LOGI(kTag, "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: Full QXGA Image Uploaded to Gateway",
             static_cast<double>(esp_timer_get_time() - session_.session_start_us) / 1000.0);

    session_.last_remote_alert_end_us = esp_timer_get_time();
    alert_cycle_in_progress_ = false;
    alert_cycles_finished_++;

    if (upload_err == ESP_OK) {
        uploads_succeeded_++;
        ESP_LOGI(kTag, "session=%s cycle=%" PRIu32 " ALERT_COMPLETE outcome=success",
                 session_.session_id, session_.alert_cycle_count);
    } else {
        ESP_LOGW(kTag, "session=%s cycle=%" PRIu32 " ALERT_COMPLETE outcome=upload_failed",
                 session_.session_id, session_.alert_cycle_count);
    }
}

void DoorbellController::process_button_press_event(int64_t event_time_us)
{
    session_.press_count++;
    session_.last_button_press_us = event_time_us;
    session_.touch_activity(event_time_us);

    const int64_t elapsed_since_last_alert_start_us = (session_.last_remote_alert_start_us > 0)
                                                           ? (event_time_us - session_.last_remote_alert_start_us)
                                                           : 0;
    const double elapsed_sec = static_cast<double>(elapsed_since_last_alert_start_us) / 1000000.0;
    const bool cooldown_expired = (session_.last_remote_alert_start_us > 0) &&
                                  (elapsed_since_last_alert_start_us >= VisitorSession::kRealertCooldownUs);
    const bool under_cycle_limit = (session_.alert_cycle_count < VisitorSession::kMaxAlertCyclesPerSession);

    if (!under_cycle_limit) {
        ESP_LOGI(kTag, "session=%s cycle=%" PRIu32 " BUTTON_PRESS local=retrigger remote=suppressed reason=max_cycles elapsed=%.1fs",
                 session_.session_id, session_.alert_cycle_count, elapsed_sec);
        return;
    }

    if (alert_cycle_in_progress_ || !cooldown_expired) {
        session_.followup_pending = true;
        ESP_LOGI(kTag, "session=%s cycle=%" PRIu32 " BUTTON_PRESS local=retrigger remote=pending reason=%s elapsed=%.1fs",
                 session_.session_id, session_.alert_cycle_count,
                 alert_cycle_in_progress_ ? "pipeline_busy" : "cooldown_active",
                 elapsed_sec);
    } else {
        ESP_LOGI(kTag, "session=%s cycle=%" PRIu32 " BUTTON_PRESS local=retrigger remote=execute_now elapsed=%.1fs",
                 session_.session_id, session_.alert_cycle_count, elapsed_sec);
        const char *firmware_version = esp_app_get_description()->version;
        intercom_.stop();
        execute_alert_cycle("DOORBELL_REPRESS", firmware_version);
        if (connectivity_.connected()) {
            (void)intercom_.start(session_.session_id);
        }
        set_state(DeviceState::ListeningForReply);
    }
}

void DoorbellController::handle_ptt_session()
{
    set_state(DeviceState::WifiReconnect);
    const int connect_timeout_ms = static_cast<int>((std::min)(
        10000000LL, session_.remaining_us(esp_timer_get_time())) / 1000LL);
    if (!connectivity_.connected() &&
        connectivity_.connect(connect_timeout_ms) != ESP_OK) {
        ESP_LOGW(kTag, "Reply window running without network connectivity");
    }

    const esp_err_t intercom_err = connectivity_.connected()
                                        ? intercom_.start(session_.session_id)
                                        : ESP_ERR_INVALID_STATE;
    if (intercom_err != ESP_OK) {
        ESP_LOGW(kTag, "Intercom control plane unavailable: %s",
                 esp_err_to_name(intercom_err));
    }
    session_.touch_activity(esp_timer_get_time());
    set_state(DeviceState::ListeningForReply);
    const char *firmware_version = esp_app_get_description()->version;

    while (!session_.is_expired(esp_timer_get_time())) {
        IntercomCommand command{};
        if (intercom_.receive(command, 0)) {
            const int64_t command_time_us = esp_timer_get_time();
            session_.touch_activity(command_time_us);
            if (command.type == IntercomCommandType::StartReply) {
                audio_.stop();
                session_.ptt_active = true;
                set_state(DeviceState::ReplyArmed);
                ESP_LOGI(kTag, "Homeowner PTT armed; visitor microphone muted");
            } else if (command.type == IntercomCommandType::CancelReply) {
                session_.ptt_active = false;
                set_state(DeviceState::ListeningForReply);
                ESP_LOGI(kTag, "Homeowner PTT cancelled");
            } else if (command.type == IntercomCommandType::PlayAudio) {
                audio_.stop();
                session_.ptt_active = true;
                set_state(DeviceState::PlayingReply);
                const std::uint32_t max_playback_ms = static_cast<std::uint32_t>(
                    session_.remaining_us(esp_timer_get_time()) / 1000LL);
                const esp_err_t play_err = intercom_.play_wav(
                    command, max_playback_ms);
                if (play_err != ESP_OK) {
                    ESP_LOGE(kTag, "Homeowner reply playback failed: %s",
                             esp_err_to_name(play_err));
                }
                session_.ptt_active = false;
                session_.touch_activity(esp_timer_get_time());
                set_state(DeviceState::ListeningForReply);
            }
        }

        // 1. Drain latest button event mailbox (length-1 overwrite queue)
        SystemEvent btn_ev;
        if (button_mailbox_ && xQueueReceive(button_mailbox_, &btn_ev, 0) == pdTRUE) {
            const int64_t now_us = esp_timer_get_time();
            process_button_press_event(now_us);
        }

        // 2. Process non-coalescible lifecycle events from FIFO event_queue_
        SystemEvent event;
        if (xQueueReceive(event_queue_, &event, pdMS_TO_TICKS(50)) == pdTRUE) {
            const int64_t now_us = esp_timer_get_time();
            if (event.type == EventType::ButtonPress) {
                process_button_press_event(now_us);
            }
        }

        // 3. Check if a follow-up alert cycle is pending and eligible
        const int64_t now_us = esp_timer_get_time();
        if (session_.followup_pending && !alert_cycle_in_progress_) {
            const int64_t cooldown_elapsed_us = (session_.last_remote_alert_start_us > 0)
                                                     ? (now_us - session_.last_remote_alert_start_us)
                                                     : 0;

            if (session_.last_remote_alert_start_us > 0 &&
                cooldown_elapsed_us >= VisitorSession::kRealertCooldownUs &&
                session_.alert_cycle_count < VisitorSession::kMaxAlertCyclesPerSession) {
                ESP_LOGI(kTag, "session=%s cycle=%" PRIu32 " ALERT_START_PENDING type=DOORBELL_REPRESS elapsed_from_first_press=%.1fs",
                         session_.session_id, session_.alert_cycle_count + 1,
                         static_cast<double>(cooldown_elapsed_us) / 1000000.0);
                intercom_.stop();
                execute_alert_cycle("DOORBELL_REPRESS", firmware_version);
                if (connectivity_.connected()) {
                    (void)intercom_.start(session_.session_id);
                }
                set_state(DeviceState::ListeningForReply);
            }
        }
    }

    intercom_.stop();

    ESP_LOGI(kTag, "session=%s VISITOR_SESSION_EXPIRED total_presses=%" PRIu32 " cycles_started=%" PRIu32 " cycles_finished=%" PRIu32 " uploads_succeeded=%" PRIu32 " duration=%.1fs",
             session_.session_id, session_.press_count, alert_cycles_started_,
             alert_cycles_finished_, uploads_succeeded_,
             static_cast<double>(esp_timer_get_time() - session_.session_start_us) / 1000000.0);
}

[[noreturn]] void DoorbellController::sleep()
{
    set_state(DeviceState::PreparingSleep);
    session_.followup_pending = false;
    stop_button_monitor();
    audio_.stop();
    intercom_.stop();
    (void)connectivity_.shutdown();
    ring_fade_stop();
    gpio_set_level(STATUS_LED_PIN, 0);

    set_state(DeviceState::WaitingForRelease);
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
        sleep();
    }

    if (reason != WakeReason::Button && reason != WakeReason::Motion) {
        sleep();
    }

    // Initialize Visitor Session
    session_ = VisitorSession{};
    make_event_id(session_.session_id);
    session_.session_start_us = started_us;
    session_.last_button_press_us = started_us;
    session_.touch_activity(started_us);
    session_.press_count = 1;

    const char *event_type = reason == WakeReason::Button
                                 ? "DOORBELL_PRESS"
                                 : "PIR_MOTION";
    const char *firmware_version = esp_app_get_description()->version;
    ESP_LOGI(kTag, "Session started id=%s device=%s firmware=%s",
             session_.session_id, DEVICE_ID, firmware_version);

    // Start background button monitor task to capture represses while awake
    start_button_monitor();

    // Execute Alert Cycle #1
    execute_alert_cycle(event_type, firmware_version);
    log_elapsed("initial alert cycle finished", started_us);

    // Enter PTT session / event loop
    handle_ptt_session();

    sleep();
}


}  // namespace doorbell
