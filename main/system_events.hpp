#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


namespace doorbell {

/**
 * High-level System Event Types dispatched across tasks.
 */
enum class EventType : std::uint8_t {
    ButtonPress,
    ButtonRelease,
    PirMotion,
    EarlyNotifyAck,
    EarlyNotifyFailed,
    CameraFrameReady,
    CameraCaptureFailed,
    UploadSuccess,
    UploadFailed,
    PttAudioReceived,
    PttSessionClose,
    SubsystemTimeout,
    HardSessionTimeout
};

/**
 * Strongly typed system event payload passed via FreeRTOS queue.
 */
struct SystemEvent {
    EventType type;
    std::uint32_t timestamp_ms;
    esp_err_t status{ESP_OK};
    void *extra_data{nullptr};
};

/**
 * Outcome tracking for remote alert cycles.
 */
enum class AlertCycleOutcome : std::uint8_t {
    Success,
    WifiFailure,
    CameraFailure,
    UploadFailure,
    SessionDeadline
};

/**
 * Visitor Session state tracking.
 * Encapsulates multi-press interactions within a bounded time window.
 */
struct VisitorSession {
    char session_id[33]{0};
    std::uint32_t press_count{0};
    std::uint32_t alert_cycle_count{0};
    int64_t session_start_us{0};
    int64_t last_activity_us{0};
    int64_t last_remote_alert_us{0};

    bool early_notified{false};
    bool image_captured{false};
    bool image_uploaded{false};
    bool ptt_active{false};
    bool followup_pending{false};

    static constexpr int64_t kPttIdleTimeoutUs = 60000000LL;       // 60 seconds
    static constexpr int64_t kSessionAbsoluteMaxUs = 90000000LL;   // 90 seconds hard max
    static constexpr int64_t kRealertCooldownUs = 15000000LL;      // 15 seconds re-alert cooldown
    static constexpr std::uint32_t kMaxAlertCyclesPerSession = 2;  // Max 2 remote alert cycles per session

    int64_t hard_deadline_us() const {
        if (session_start_us == 0) return 0;
        return session_start_us + kSessionAbsoluteMaxUs;
    }

    int64_t remaining_us(int64_t current_time_us) const {
        if (session_start_us == 0) return 0;
        const int64_t deadline = hard_deadline_us();
        return (std::max)(0LL, deadline - current_time_us);
    }

    bool is_expired(int64_t current_time_us) const {
        if (session_start_us == 0) return true;
        const int64_t hard_limit = hard_deadline_us();
        const int64_t idle_limit = last_activity_us + kPttIdleTimeoutUs;
        const int64_t effective_end = (std::min)(idle_limit, hard_limit);
        return current_time_us >= effective_end;
    }

    void touch_activity(int64_t current_time_us) {
        last_activity_us = current_time_us;
    }
};

/**
 * Safety & Invariant Enforcement Assertions
 */
inline bool validate_invariant_pwr_01(bool wifi_rf_active, bool cam_pwr_en) {
    // PWR-01: Wi-Fi RF active and CAM_PWR_EN must never be true simultaneously
    return !(wifi_rf_active && cam_pwr_en);
}

}  // namespace doorbell
