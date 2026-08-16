#include "doorbell_controller.hpp"

#include <cstdio>
#include <cstring>
#include <inttypes.h>
#include <new>

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
constexpr std::uint32_t kVisitorRecordingMaxMs =
    DoorbellPolicy::kVisitorRecordingMaxMs;
constexpr std::uint32_t kVisitorHoldMinimumMs =
    DoorbellPolicy::kVisitorHoldMinimumMs;
constexpr std::uint32_t kRepressChimeHoldHandoffMs =
    DoorbellPolicy::kRepressChimeHoldHandoffMs;
constexpr std::uint32_t kChimeDrainBeforeSleepMs = 4000;
portMUX_TYPE s_button_turn_lock = portMUX_INITIALIZER_UNLOCKED;

class CameraSpeakerPowerBoundary final {
public:
    CameraSpeakerPowerBoundary()
    {
        chime_player_set_camera_overlap_mode(true);
    }

    ~CameraSpeakerPowerBoundary()
    {
        release();
    }

    void release()
    {
        if (active_) {
            chime_player_set_camera_overlap_mode(false);
            active_ = false;
        }
    }

    CameraSpeakerPowerBoundary(const CameraSpeakerPowerBoundary &) = delete;
    CameraSpeakerPowerBoundary &operator=(const CameraSpeakerPowerBoundary &) = delete;

private:
    bool active_{true};
};

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
    case DeviceState::RecordingVisitor:
        return "RECORDING_VISITOR";
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
    std::uint32_t recorded_duration_ms;
    std::int64_t session_start_us;
    std::atomic_bool *capture_window_active;
    esp_err_t result{ESP_FAIL};
};

void record_greeting_task(void *arg)
{
    auto *context = static_cast<GreetingCaptureTaskContext *>(arg);

    // The first chime owns I2S until its final sample. A visitor who wants to
    // leave a message keeps holding; the microphone starts immediately after.
    while (chime_player_is_playing()) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    ESP_LOGI(kTag,
             "[MONOTONIC_TIMING] t=%.1f ms | STAGE_START: Visitor microphone recording",
             static_cast<double>(esp_timer_get_time() - context->session_start_us) /
                 1000.0);
    (void)ring_animation_start(100, context->duration_ms, 150);
    context->result = context->audio_service->record_button_hold(
        *context->greeting, context->duration_ms, kVisitorHoldMinimumMs,
        context->recorded_duration_ms);
    ring_fade_stop();
    context->capture_window_active->store(false);
    xSemaphoreGive(context->completion);
    vTaskDelete(nullptr);
}

struct FollowupTurn {
    AudioService *audio_service;
    DoorbellController *controller;
    std::atomic_bool *capture_window_active;
    std::atomic_bool *shutting_down;
    std::atomic_uint32_t *latest_press_number;
    std::uint32_t press_number;
    std::int64_t press_started_us;
    bool dispatch_alerts;
    std::uint32_t recorded_duration_ms{0};
    std::uint32_t max_duration_ms{0};
    RecordedAudio recording;
    esp_err_t result{ESP_FAIL};
};

void complete_followup_turn(FollowupTurn *turn)
{
    SystemEvent ready{EventType::VisitorRecordingReady,
                      static_cast<std::uint32_t>(esp_timer_get_time() / 1000LL),
                      turn->result, turn};
    (void)turn->controller->post_event(ready);
    vTaskDelete(nullptr);
}

void record_followup_turn_task(void *arg)
{
    auto *turn = static_cast<FollowupTurn *>(arg);
    if (turn->shutting_down->load()) {
        turn->result = ESP_ERR_INVALID_STATE;
        complete_followup_turn(turn);
        return;
    }

    // A tap must never reserve the microphone or make a later press's chime
    // interruptible. Wait for this exact down-edge to remain continuously held
    // through the handoff threshold before touching audio arbitration.
    const std::int64_t handoff_us =
        turn->press_started_us +
        static_cast<std::int64_t>(kRepressChimeHoldHandoffMs) * 1000LL;
    std::int64_t release_candidate_us = 0;
    while (esp_timer_get_time() < handoff_us &&
           !turn->shutting_down->load()) {
        const std::int64_t now_us = esp_timer_get_time();
        if (turn->latest_press_number->load() != turn->press_number) {
            turn->result = ESP_OK;
            complete_followup_turn(turn);
            return;
        }
        if (gpio_get_level(DOORBELL_BUTTON_PIN) == 0) {
            release_candidate_us = 0;
        } else if (release_candidate_us == 0) {
            release_candidate_us = now_us;
        } else if (now_us - release_candidate_us >= 20000LL) {
            turn->result = ESP_OK;
            complete_followup_turn(turn);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (turn->shutting_down->load()) {
        turn->result = ESP_ERR_INVALID_STATE;
        complete_followup_turn(turn);
        return;
    }
    if (turn->max_duration_ms < kVisitorHoldMinimumMs ||
        turn->latest_press_number->load() != turn->press_number ||
        gpio_get_level(DOORBELL_BUTTON_PIN) != 0) {
        turn->result = ESP_OK;
        complete_followup_turn(turn);
        return;
    }

    // Only a qualified continuous hold may wait for the microphone window.
    // Wait while it remains the latest physical press instead of timing out
    // behind an older turn and reporting a false recording failure.
    const std::int64_t turn_deadline_us =
        turn->press_started_us +
        static_cast<std::int64_t>(turn->max_duration_ms) * 1000LL;
    bool acquired = false;
    while (!turn->shutting_down->load() &&
           turn->latest_press_number->load() == turn->press_number &&
           esp_timer_get_time() < turn_deadline_us) {
        if (gpio_get_level(DOORBELL_BUTTON_PIN) != 0) {
            turn->result = ESP_OK;
            complete_followup_turn(turn);
            return;
        }
        bool expected = false;
        acquired = turn->capture_window_active->compare_exchange_strong(
            expected, true);
        if (acquired) break;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (!acquired) {
        turn->result = turn->shutting_down->load()
                           ? ESP_ERR_INVALID_STATE
                           : ESP_OK;
        complete_followup_turn(turn);
        return;
    }

    if (turn->latest_press_number->load() != turn->press_number ||
        gpio_get_level(DOORBELL_BUTTON_PIN) != 0) {
        turn->capture_window_active->store(false);
        turn->result = ESP_OK;
        complete_followup_turn(turn);
        return;
    }

    ESP_LOGI(kTag,
             "Repress press=%" PRIu32 " held continuously for %u ms; arming visitor microphone",
             turn->press_number,
             static_cast<unsigned>(kRepressChimeHoldHandoffMs));
    bool yielded_chime = false;
    portENTER_CRITICAL(&s_button_turn_lock);
    if (turn->latest_press_number->load() == turn->press_number &&
        gpio_get_level(DOORBELL_BUTTON_PIN) == 0 &&
        chime_player_is_interruptible()) {
        chime_player_request_stop();
        yielded_chime = true;
    }
    portEXIT_CRITICAL(&s_button_turn_lock);
    if (yielded_chime) {
        ESP_LOGI(kTag, "Repress press=%" PRIu32
                       " yielding its local chime to visitor microphone",
                 turn->press_number);
    }
    while (chime_player_is_playing() &&
           !turn->shutting_down->load() &&
           turn->latest_press_number->load() == turn->press_number) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (turn->shutting_down->load() ||
        turn->latest_press_number->load() != turn->press_number ||
        gpio_get_level(DOORBELL_BUTTON_PIN) != 0) {
        turn->capture_window_active->store(false);
        turn->result = turn->shutting_down->load()
                           ? ESP_ERR_INVALID_STATE
                           : ESP_OK;
        complete_followup_turn(turn);
        return;
    }

    // record_button_hold() performs a final level check so a release during
    // I2S handoff remains a normal tap without an empty WAV.
    (void)ring_animation_start(100, turn->max_duration_ms, 150);
    turn->result = turn->audio_service->record_button_hold(
        turn->recording, turn->max_duration_ms, kVisitorHoldMinimumMs,
        turn->recorded_duration_ms);
    ring_fade_stop();
    turn->capture_window_active->store(false);
    complete_followup_turn(turn);
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
    event_queue_ = xQueueCreate(24, sizeof(SystemEvent));
    if (!event_queue_) {
        ESP_LOGE(kTag, "Failed to create system event queue");
    }
}

DoorbellController::~DoorbellController()
{
    if (event_queue_) {
        vQueueDelete(event_queue_);
        event_queue_ = nullptr;
    }
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

esp_err_t DoorbellController::upload_with_retry(const CapturedImage *image,
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
    const char *firmware_version, bool dispatch_alerts, int timeout_ms)
{
    esp_err_t err = ESP_FAIL;
    for (unsigned attempt = 1; attempt <= kTriggerAttempts; ++attempt) {
        ESP_LOGI(kTag, "Early trigger attempt %u/%u (timeout=%d ms)", attempt,
                 kTriggerAttempts, timeout_ms);
        err = connectivity_.trigger(event_id, event_type, device_id,
                                    firmware_version, dispatch_alerts,
                                    timeout_ms);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        if (attempt < kTriggerAttempts) {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    return err;
}

esp_err_t DoorbellController::handle_early_notify(
    const char *event_id, const char *event_type, const char *firmware_version,
    bool dispatch_alerts, int remaining_ms)
{
    set_state(DeviceState::EarlyNotify);
    int wifi_timeout = (std::min)(10000, remaining_ms);
    esp_err_t err = connectivity_.connect(wifi_timeout);
    if (err == ESP_OK) {
        int trigger_timeout = (std::min)(5000, remaining_ms);
        esp_err_t trigger_err = trigger_with_retry(
            event_id, event_type, DEVICE_ID, firmware_version,
            dispatch_alerts, trigger_timeout);
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

esp_err_t DoorbellController::handle_upload(const CapturedImage *image,
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
        if (image && image->valid()) session_.image_uploaded = true;
        ESP_LOGI(kTag, "%s event completed (image=%u bytes, audio=%u bytes, event_id=%s)",
                 event_type,
                 static_cast<unsigned>(image && image->valid() ? image->size() : 0),
                 static_cast<unsigned>(audio && audio->valid() ? audio->size() : 0),
                 event_id);
    } else {
        ESP_LOGE(kTag, "%s event failed: %s", event_type, esp_err_to_name(err));
    }
    return err;
}

void DoorbellController::start_followup_capture(std::uint32_t press_number,
                                                std::int64_t press_started_us,
                                                bool dispatch_alerts)
{
    auto *turn = new (std::nothrow) FollowupTurn{
        .audio_service = &audio_,
        .controller = this,
        .capture_window_active = &visitor_capture_window_active_,
        .shutting_down = &shutting_down_,
        .latest_press_number = &next_press_number_,
        .press_number = press_number,
        .press_started_us = press_started_us,
        .dispatch_alerts = dispatch_alerts,
        .recorded_duration_ms = 0,
        .max_duration_ms = static_cast<std::uint32_t>((std::min)(
            static_cast<std::int64_t>(kVisitorRecordingMaxMs),
            session_.remaining_us(press_started_us) / 1000LL)),
        .recording = {},
        .result = ESP_FAIL,
    };
    if (!turn) {
        ESP_LOGE(kTag, "Could not allocate visitor turn metadata");
        pending_button_events_.fetch_sub(1);
        if (dispatch_alerts) {
            repress_alert_slot_available_.store(true);
        }
        return;
    }
    outstanding_followup_turns_.fetch_add(1);

    SystemEvent started{EventType::ButtonPress,
                        static_cast<std::uint32_t>(press_started_us / 1000LL),
                        ESP_OK, turn};
    if (post_event(started) != ESP_OK) {
        pending_button_events_.fetch_sub(1);
        turn->dispatch_alerts = false;
    }
    if (xTaskCreate(record_followup_turn_task, "visitor_hold", 4096, turn,
                    configMAX_PRIORITIES - 4, nullptr) != pdPASS) {
        turn->result = ESP_ERR_NO_MEM;
        SystemEvent ready{EventType::VisitorRecordingReady,
                          static_cast<std::uint32_t>(esp_timer_get_time() / 1000LL),
                          turn->result, turn};
        (void)post_event(ready);
    }
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
            } else if ((now_us - candidate_start_us) >= 20000LL &&
                       candidate_btn_level != stable_btn_level) {
                stable_btn_level = candidate_btn_level;
                if (stable_btn_level == 1) {
                    button_released = true;
                } else if (stable_btn_level == 0 && button_released) {
                    button_released = false;
                    ESP_LOGI(kTag, "⚡ Button repress detected by ButtonMonitor!");

                    // Publish the new physical edge before touching either
                    // side of I2S. Older hold tasks must observe that they no
                    // longer own the button, and any old microphone capture
                    // must stop before this press can qualify for a new turn.
                    portENTER_CRITICAL(&s_button_turn_lock);
                    const std::uint32_t press_number =
                        self->next_press_number_.fetch_add(1) + 1;
                    portEXIT_CRITICAL(&s_button_turn_lock);
                    if (self->audio_.capture_active()) {
                        self->audio_.request_capture_stop();
                    }

                    // Every new down-edge is one press/possible visitor turn.
                    // Repress audio is opportunistic and never queues behind
                    // an active microphone or homeowner reply. If the local
                    // chime already owns I2S, retrigger it in place so rapid
                    // presses remain individually perceptible.
                    (void)ring_animation_start(150, 1000, 300);
                    const esp_err_t chime_err =
                        chime_player_play_repress_async();
                    if (chime_err != ESP_OK) {
                        ESP_LOGW(kTag, "Could not start repress chime: %s",
                                 esp_err_to_name(chime_err));
                    }
                    // Publish the pending edge before attempting to reserve
                    // the immediate-alert slot. This ordering prevents the
                    // controller from reopening the slot between reservation
                    // and FIFO insertion.
                    self->pending_button_events_.fetch_add(1);
                    bool expected = true;
                    const bool dispatch_alerts =
                        self->repress_alert_slot_available_
                            .compare_exchange_strong(expected, false);
                    self->start_followup_capture(
                        press_number, now_us, dispatch_alerts);
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

void DoorbellController::execute_alert_cycle(const char *event_type,
                                             const char *firmware_version,
                                             std::uint32_t press_number,
                                             bool dispatch_alerts)
{
    const int64_t now_start_us = esp_timer_get_time();
    int remaining_ms = static_cast<int>(session_.remaining_us(now_start_us) / 1000LL);
    if (session_.is_expired(now_start_us) || remaining_ms <= 0) {
        ESP_LOGW(kTag, "session=%s cycle=%" PRIu32 " ALERT_SKIPPED reason=session_deadline_reached",
                 session_.session_id, session_.alert_cycle_count + 1);
        return;
    }

    // Keep every chime request on the bounded camera-safe profile until this
    // cycle has either completed camera work or returned through an error path.
    CameraSpeakerPowerBoundary speaker_boundary;

    session_.alert_cycle_count++;
    alert_cycles_started_++;

    char cycle_event_id[40];
    std::snprintf(cycle_event_id, sizeof(cycle_event_id), "%s-%" PRIu32,
                  session_.session_id, press_number);

    ESP_LOGI(kTag, "[MONOTONIC_TIMING] t=%.1f ms | ALERT_START session=%s event_id=%s cycle=%" PRIu32 " type=%s remaining_time=%.1fs",
             static_cast<double>(now_start_us - session_.session_start_us) / 1000.0,
             session_.session_id, cycle_event_id, press_number, event_type,
             static_cast<double>(remaining_ms) / 1000.0);

    RecordedAudio greeting;
    // The initial turn shares this cycle's media upload. Follow-up turns are
    // captured from their own down-edges and uploaded independently.
    const bool record_visitor_greeting =
        session_.alert_cycle_count == 1 &&
        std::strcmp(event_type, "DOORBELL_PRESS") == 0;
    const int greeting_budget_ms = static_cast<int>((std::min)(
        static_cast<int64_t>(kVisitorRecordingMaxMs),
        (std::max)(0LL, session_.remaining_us(esp_timer_get_time()) / 1000LL -
                            10000LL)));

    StaticSemaphore_t greeting_completion_storage{};
    SemaphoreHandle_t greeting_completion = nullptr;
    GreetingCaptureTaskContext greeting_context{};
    bool greeting_task_started = false;
    if (record_visitor_greeting &&
        greeting_budget_ms >= static_cast<int>(kVisitorHoldMinimumMs)) {
        visitor_capture_window_active_.store(true);
        greeting_completion =
            xSemaphoreCreateBinaryStatic(&greeting_completion_storage);
        greeting_context = {
            .audio_service = &audio_,
            .greeting = &greeting,
            .completion = greeting_completion,
            .duration_ms = static_cast<std::uint32_t>(greeting_budget_ms),
            .recorded_duration_ms = 0,
            .session_start_us = session_.session_start_us,
            .capture_window_active = &visitor_capture_window_active_,
            .result = ESP_FAIL,
        };
        greeting_task_started =
            xTaskCreate(record_greeting_task, "visitor_audio", 4096,
                        &greeting_context, configMAX_PRIORITIES - 4,
                        nullptr) == pdPASS;
        if (!greeting_task_started) {
            ESP_LOGW(kTag,
                     "Could not start immediate visitor recording; using sequential fallback");
        }
    }

    auto finish_greeting_task = [&]() {
        if (!greeting_task_started) {
            return;
        }
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
    };

    const bool initial_press = std::strcmp(event_type, "DOORBELL_PRESS") == 0;

    // Represses that can be handled immediately notify before optional snapshot
    // work. A queued lifecycle event is still registered, but dispatchAlerts=false
    // permanently prevents it from turning into a delayed whole-home chime.
    if (!initial_press) {
        (void)handle_early_notify(cycle_event_id, event_type, firmware_version,
                                  dispatch_alerts, remaining_ms);
        ESP_LOGI(kTag,
                 "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: Repress registered (alerts=%s)",
                 static_cast<double>(esp_timer_get_time() -
                                     session_.session_start_us) /
                     1000.0,
                 dispatch_alerts ? "immediate" : "suppressed");
    }

    int64_t now_us = esp_timer_get_time();
    remaining_ms = static_cast<int>(session_.remaining_us(now_us) / 1000LL);
    if (remaining_ms <= 0) {
        ESP_LOGW(kTag,
                 "session=%s cycle=%" PRIu32 " ALERT_ABORTED stage=pre_capture reason=session_deadline_reached",
                 session_.session_id, session_.alert_cycle_count);
        finish_greeting_task();
        visitor_capture_window_active_.store(false);
        alert_cycles_finished_++;
        return;
    }

    // The first press reaches this boundary before any Wi-Fi connection or
    // HTTP request. A camera-safe -12 dB chime may continue through capture. A stale
    // snapshot cycle can begin with a normal -6 dB repress that was claimed
    // before this task classified it; only that higher-power stream must drain.
    if (chime_player_is_playing() &&
        !chime_player_camera_overlap_active()) {
        ESP_LOGI(kTag,
                 "Waiting for non-overlap local chime to finish before camera power-up");
        const esp_err_t chime_idle_err = chime_player_wait_until_idle(
            pdMS_TO_TICKS(static_cast<std::uint32_t>(remaining_ms)));
        if (chime_idle_err != ESP_OK) {
            ESP_LOGE(kTag,
                     "session=%s cycle=%" PRIu32 " ALERT_FAILED reason=speaker_camera_boundary_timeout",
                     session_.session_id, session_.alert_cycle_count);
            finish_greeting_task();
            visitor_capture_window_active_.store(false);
            alert_cycles_finished_++;
            return;
        }
    } else {
        ESP_LOGI(kTag,
                 "Camera-safe local chime may continue while camera starts");
    }
    ESP_LOGI(kTag,
             "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: Camera-safe speaker boundary ready",
             static_cast<double>(esp_timer_get_time() - session_.session_start_us) /
                 1000.0);

    now_us = esp_timer_get_time();
    remaining_ms = static_cast<int>(session_.remaining_us(now_us) / 1000LL);
    if (remaining_ms <= 0) {
        ESP_LOGW(kTag,
                 "session=%s cycle=%" PRIu32 " ALERT_ABORTED stage=speaker_drain reason=session_deadline_reached",
                 session_.session_id, session_.alert_cycle_count);
        finish_greeting_task();
        visitor_capture_window_active_.store(false);
        alert_cycles_finished_++;
        return;
    }

    // PWR-01 boundary: ensure Wi-Fi RF is off before camera power-up.
    const esp_err_t shutdown_err = handle_rf_quiesce();
    if (shutdown_err != ESP_OK) {
        ESP_LOGE(kTag, "session=%s cycle=%" PRIu32 " ALERT_FAILED reason=rf_quiesce_failed",
                 session_.session_id, session_.alert_cycle_count);
        finish_greeting_task();
        visitor_capture_window_active_.store(false);
        alert_cycles_finished_++;
        return;
    }
    ESP_LOGI(kTag, "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: RF Quiesced (Wi-Fi disabled for camera capture)",
             static_cast<double>(esp_timer_get_time() - session_.session_start_us) / 1000.0);

    // Capture the photo while the visitor greeting task waits for I2S or records.
    CapturedImage image;
    const esp_err_t capture_err = handle_camera_capture(image, remaining_ms);
    speaker_boundary.release();
    if (capture_err == ESP_OK) last_snapshot_capture_us_ = esp_timer_get_time();

    if (capture_err == ESP_OK && image.valid()) {
        ESP_LOGI(kTag,
                 "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: Camera Photo Captured & Copied to PSRAM (SHUTTER CLOSE)",
                 static_cast<double>(esp_timer_get_time() -
                                     session_.session_start_us) /
                     1000.0);
    } else {
        ESP_LOGE(kTag,
                 "session=%s cycle=%" PRIu32 " ALERT_FAILED reason=camera_capture_failed",
                 session_.session_id, session_.alert_cycle_count);
    }

    // The first press deliberately captures before starting RF. This prevents
    // Wi-Fi/webhook latency from moving the shutter toward button release.
    if (initial_press) {
        now_us = esp_timer_get_time();
        remaining_ms = static_cast<int>(session_.remaining_us(now_us) / 1000LL);
        if (remaining_ms > 0) {
            (void)handle_early_notify(cycle_event_id, event_type,
                                      firmware_version, true, remaining_ms);
            ESP_LOGI(kTag,
                     "[MONOTONIC_TIMING] t=%.1f ms | STAGE_COMPLETE: Initial notification sent after shutter close",
                     static_cast<double>(esp_timer_get_time() -
                                         session_.session_start_us) /
                         1000.0);
        }
    }

    // Upload the owned image before joining a held-button recording. The
    // dashboard can therefore receive the snapshot while the visitor is still
    // holding the button; any retained WAV is attached in a second request.
    now_us = esp_timer_get_time();
    remaining_ms = static_cast<int>(session_.remaining_us(now_us) / 1000LL);
    esp_err_t image_upload_err = capture_err == ESP_OK
                                     ? ESP_ERR_TIMEOUT
                                     : capture_err;
    if (image.valid() && remaining_ms > 0) {
        ESP_LOGI(kTag, "Controller stack headroom before image upload: %u bytes",
                 static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
        image_upload_err = handle_upload(
            &image, nullptr, cycle_event_id, event_type, firmware_version,
            remaining_ms);
        ESP_LOGI(kTag, "Controller stack headroom after image upload: %u bytes",
                 static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
    }
    image.reset();

    finish_greeting_task();

    if (!greeting_task_started && record_visitor_greeting &&
        greeting_budget_ms >= static_cast<int>(kVisitorHoldMinimumMs)) {
        set_state(DeviceState::RecordingVisitor);
        std::uint32_t recorded_duration_ms = 0;
        const esp_err_t audio_err = audio_.record_button_hold(
            greeting, static_cast<std::uint32_t>(greeting_budget_ms),
            kVisitorHoldMinimumMs, recorded_duration_ms);
        visitor_capture_window_active_.store(false);
        if (audio_err != ESP_OK) {
            ESP_LOGW(kTag,
                     "Visitor greeting unavailable; continuing without audio: %s",
                     esp_err_to_name(audio_err));
        }
    }

    esp_err_t audio_upload_err = ESP_OK;
    if (greeting.valid()) {
        now_us = esp_timer_get_time();
        remaining_ms = static_cast<int>(session_.remaining_us(now_us) / 1000LL);
        if (remaining_ms > 0) {
            audio_upload_err = handle_upload(
                nullptr, &greeting, cycle_event_id, event_type,
                firmware_version, remaining_ms);
        } else {
            audio_upload_err = ESP_ERR_TIMEOUT;
        }
    }
    greeting.reset();

    alert_cycles_finished_++;

    if (image_upload_err == ESP_OK && audio_upload_err == ESP_OK) {
        uploads_succeeded_++;
        ESP_LOGI(kTag, "session=%s cycle=%" PRIu32 " ALERT_COMPLETE outcome=success",
                 session_.session_id, session_.alert_cycle_count);
    } else {
        ESP_LOGW(kTag, "session=%s cycle=%" PRIu32 " ALERT_COMPLETE outcome=upload_failed",
                 session_.session_id, session_.alert_cycle_count);
    }
}

void DoorbellController::process_button_press_event(int64_t event_time_us,
                                                    void *turn_data)
{
    auto *turn = static_cast<FollowupTurn *>(turn_data);
    if (turn) {
        event_time_us = turn->press_started_us;
        pending_button_events_.fetch_sub(1);
    }
    const std::uint32_t press_number = turn ? turn->press_number
                                            : next_press_number_.fetch_add(1) + 1;
    const bool dispatch_alerts = !turn || turn->dispatch_alerts;
    session_.press_count = (std::max)(session_.press_count, press_number);
    session_.last_button_press_us = event_time_us;
    session_.touch_activity(esp_timer_get_time());

    const char *firmware_version = esp_app_get_description()->version;
    const bool snapshot_stale = last_snapshot_capture_us_ == 0 ||
        event_time_us - last_snapshot_capture_us_ >= DoorbellPolicy::kSnapshotRefreshUs;
    if (snapshot_stale) {
        ESP_LOGI(kTag,
                 "session=%s press=%" PRIu32 " followup=snapshot_refresh global_alert=%s",
                 session_.session_id, press_number,
                 dispatch_alerts ? "immediate" : "suppressed");
        intercom_.stop();
        execute_alert_cycle("DOORBELL_REPRESS", firmware_version,
                            press_number, dispatch_alerts);
        if (connectivity_.connected()) (void)intercom_.start(session_.session_id);
    } else {
        char press_id[40];
        std::snprintf(press_id, sizeof(press_id), "%s-%" PRIu32,
                      session_.session_id, press_number);
        const int remaining_ms = static_cast<int>(
            session_.remaining_us(esp_timer_get_time()) / 1000LL);
        ESP_LOGI(kTag,
                 "session=%s press=%" PRIu32 " followup=register_only snapshot=fresh global_alert=%s",
                 session_.session_id, press_number,
                 dispatch_alerts ? "immediate" : "suppressed");
        (void)handle_early_notify(press_id, "DOORBELL_REPRESS",
                                  firmware_version, dispatch_alerts,
                                  remaining_ms);
    }
    set_state(DeviceState::ListeningForReply);
}

void DoorbellController::process_visitor_recording(
    void *turn_data, const char *firmware_version)
{
    auto *turn = static_cast<FollowupTurn *>(turn_data);
    if (!turn) return;
    session_.touch_activity(esp_timer_get_time());
    char press_id[40];
    std::snprintf(press_id, sizeof(press_id), "%s-%" PRIu32,
                  session_.session_id, turn->press_number);
    if (connectivity_.connected()) {
        const esp_err_t complete_err = connectivity_.complete_press(
            press_id, turn->recorded_duration_ms);
        if (complete_err != ESP_OK) {
            ESP_LOGW(kTag, "Press completion was not acknowledged: %s",
                     esp_err_to_name(complete_err));
        }
    }
    if (turn->result == ESP_OK && turn->recording.valid()) {
        const int remaining_ms = static_cast<int>(
            session_.remaining_us(esp_timer_get_time()) / 1000LL);
        ESP_LOGI(kTag, "Uploading visitor recording for press=%" PRIu32
                       " duration=%u ms",
                 turn->press_number,
                 static_cast<unsigned>(turn->recorded_duration_ms));
        (void)handle_upload(nullptr, &turn->recording, press_id,
                            "DOORBELL_REPRESS", firmware_version, remaining_ms);
    } else if (turn->result != ESP_OK) {
        ESP_LOGW(kTag, "Visitor recording failed for press=%" PRIu32 ": %s",
                 turn->press_number, esp_err_to_name(turn->result));
    } else {
        ESP_LOGI(kTag, "Press=%" PRIu32 " was shorter than the voice threshold",
                 turn->press_number);
    }
    delete turn;
    outstanding_followup_turns_.fetch_sub(1);
    set_state(DeviceState::ListeningForReply);
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
    IntercomCommand deferred_reply{};
    bool has_deferred_reply = false;

    while (!session_.is_expired(esp_timer_get_time())) {
        IntercomCommand command{};
        bool has_command = false;
        if (has_deferred_reply && !visitor_capture_window_active_.load()) {
            command = deferred_reply;
            has_deferred_reply = false;
            has_command = true;
        } else {
            has_command = intercom_.receive(command, 0);
        }
        if (has_command) {
            repress_alert_slot_available_.store(false);
            const int64_t command_time_us = esp_timer_get_time();
            session_.touch_activity(command_time_us);
            if (command.type == IntercomCommandType::StartReply) {
                session_.ptt_active = true;
                if (visitor_capture_window_active_.load()) {
                    ESP_LOGI(kTag,
                             "Homeowner reply armed and queued behind visitor recording");
                } else {
                    audio_.stop();
                    set_state(DeviceState::ReplyArmed);
                    ESP_LOGI(kTag, "Homeowner PTT armed");
                }
            } else if (command.type == IntercomCommandType::CancelReply) {
                session_.ptt_active = false;
                has_deferred_reply = false;
                set_state(DeviceState::ListeningForReply);
                ESP_LOGI(kTag, "Homeowner PTT cancelled");
            } else if (command.type == IntercomCommandType::PlayAudio) {
                if (visitor_capture_window_active_.load() ||
                    pending_button_events_.load() != 0) {
                    deferred_reply = command;
                    has_deferred_reply = true;
                    ESP_LOGI(kTag,
                             "Homeowner playback queued behind visitor/button activity");
                } else {
                    // Homeowner speech outranks a repress acknowledgement.
                    // The first chime cannot overlap this state because the
                    // reply window opens only after its completion.
                    if (chime_player_is_interruptible()) {
                        ESP_LOGI(kTag,
                                 "Homeowner reply interrupting local repress chime");
                        chime_player_request_stop();
                        const std::int64_t chime_stop_deadline =
                            esp_timer_get_time() + 500000LL;
                        while (chime_player_is_playing() &&
                               esp_timer_get_time() < chime_stop_deadline) {
                            vTaskDelay(pdMS_TO_TICKS(10));
                        }
                    }
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
        }

        // Process lifecycle data in order. FollowupTurn::dispatch_alerts was
        // fixed at the physical edge, so this FIFO can never manufacture a
        // delayed whole-home ring from an old press.
        SystemEvent event;
        if (xQueueReceive(event_queue_, &event, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (event.type == EventType::ButtonPress) {
                process_button_press_event(
                    static_cast<int64_t>(event.timestamp_ms) * 1000LL,
                    event.extra_data);
            } else if (event.type == EventType::VisitorRecordingReady) {
                process_visitor_recording(event.extra_data, firmware_version);
            }
        }

        // Open the one-shot edge slot only when there is no earlier controller
        // work for a repress to sit behind. Recheck after publishing the slot
        // so a racing physical edge always closes it before enqueueing.
        if (pending_button_events_.load() == 0 &&
            outstanding_followup_turns_.load() == 0 &&
            uxQueueMessagesWaiting(event_queue_) == 0) {
            repress_alert_slot_available_.store(true);
            if (pending_button_events_.load() != 0 ||
                outstanding_followup_turns_.load() != 0 ||
                uxQueueMessagesWaiting(event_queue_) != 0) {
                repress_alert_slot_available_.store(false);
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
    shutting_down_.store(true);
    stop_button_monitor();
    if (chime_player_is_playing()) {
        ESP_LOGI(kTag,
                 "Session ended with a local chime active; draining it before sleep");
        const esp_err_t drain_err = chime_player_wait_until_idle(
            pdMS_TO_TICKS(kChimeDrainBeforeSleepMs));
        if (drain_err != ESP_OK) {
            // A wedged peripheral must not prevent bounded shutdown forever.
            // This is a fault fallback, not a normal chime interruption path.
            ESP_LOGE(kTag,
                     "Local chime did not drain before sleep; forcing audio shutdown");
            chime_player_request_stop();
            const std::int64_t stop_deadline_us =
                esp_timer_get_time() + 500000LL;
            while (chime_player_is_playing() &&
                   esp_timer_get_time() < stop_deadline_us) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }
    audio_.request_capture_stop();
    const std::int64_t capture_stop_deadline = esp_timer_get_time() + 500000LL;
    while ((audio_.capture_active() || visitor_capture_window_active_.load()) &&
           esp_timer_get_time() < capture_stop_deadline) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    visitor_capture_window_active_.store(false);
    audio_.stop();
    intercom_.stop();
    if (connectivity_.connected() && session_.session_id[0] != '\0') {
        const esp_err_t close_err = connectivity_.close_session(session_.session_id);
        if (close_err != ESP_OK) {
            ESP_LOGW(kTag, "Session close was not acknowledged: %s",
                     esp_err_to_name(close_err));
        }
    }
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
        // The first photo must not wait for a full-power chime. Arm the bounded
        // overlap profile before claiming I2S so this complete waveform can
        // safely continue through camera startup and capture.
        chime_player_set_camera_overlap_mode(true);
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
    next_press_number_.store(1);

    const char *event_type = reason == WakeReason::Button
                                 ? "DOORBELL_PRESS"
                                 : "PIR_MOTION";
    const char *firmware_version = esp_app_get_description()->version;
    ESP_LOGI(kTag, "Session started id=%s device=%s firmware=%s",
             session_.session_id, DEVICE_ID, firmware_version);

    // Start background button monitor task to capture represses while awake
    start_button_monitor();

    // Execute Alert Cycle #1
    execute_alert_cycle(event_type, firmware_version, 1);
    log_elapsed("initial alert cycle finished", started_us);

    // Enter PTT session / event loop
    if (pending_button_events_.load() == 0 &&
        outstanding_followup_turns_.load() == 0 &&
        uxQueueMessagesWaiting(event_queue_) == 0) {
        repress_alert_slot_available_.store(true);
        if (pending_button_events_.load() != 0 ||
            outstanding_followup_turns_.load() != 0 ||
            uxQueueMessagesWaiting(event_queue_) != 0) {
            repress_alert_slot_available_.store(false);
        }
    }
    handle_ptt_session();

    sleep();
}


}  // namespace doorbell
