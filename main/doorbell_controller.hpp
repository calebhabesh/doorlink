#pragma once

#include <atomic>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "services/camera_service.hpp"
#include "services/audio_service.hpp"
#include "services/connectivity_manager.hpp"
#include "services/intercom_service.hpp"
#include "services/power_manager.hpp"
#include "system_events.hpp"

namespace doorbell {

enum class DeviceState : std::uint8_t {
    Booting,
    EarlyNotify,
    RfQuiesce,
    CameraPowerUp,
    Capturing,
    CameraPowerDown,
    RecordingVisitor,
    WifiReconnect,
    Uploading,
    ListeningForReply,
    ReplyArmed,
    PlayingReply,
    PreparingSleep,
    WaitingForRelease
};

class DoorbellController final {
public:
    DoorbellController();
    ~DoorbellController();

    [[noreturn]] void run();
    esp_err_t post_event(const SystemEvent &event);

private:
    void set_state(DeviceState state);
    esp_err_t handle_early_notify(const char *event_id, const char *event_type,
                                  const char *firmware_version,
                                  bool dispatch_alerts = true,
                                  int remaining_ms = 30000);
    esp_err_t handle_rf_quiesce();
    esp_err_t handle_camera_capture(CapturedImage &image, int remaining_ms = 4000);
    esp_err_t handle_upload(const CapturedImage *image,
                             const RecordedAudio *audio,
                             const char *event_id,
                             const char *event_type,
                             const char *firmware_version,
                             int remaining_ms = 30000);
    void execute_alert_cycle(const char *event_type, const char *firmware_version,
                             std::uint32_t press_number,
                             bool dispatch_alerts = true);
    void process_button_press_event(int64_t event_time_us, void *turn_data = nullptr);
    void process_visitor_recording(void *turn_data, const char *firmware_version);
    void start_followup_capture(std::uint32_t press_number,
                                std::int64_t press_started_us,
                                bool dispatch_alerts);
    void handle_ptt_session();

    void start_button_monitor();
    void stop_button_monitor();

    esp_err_t capture_with_retry(CapturedImage &image, int timeout_ms = 4000);
    esp_err_t trigger_with_retry(const char *event_id,
                                 const char *event_type,
                                 const char *device_id,
                                 const char *firmware_version,
                                 bool dispatch_alerts,
                                 int timeout_ms = 5000);
    esp_err_t upload_with_retry(const CapturedImage *image,
                                const RecordedAudio *audio,
                                const char *event_type,
                                const char *event_id,
                                const char *device_id,
                                const char *firmware_version,
                                int timeout_ms = 30000);
    [[noreturn]] void sleep();


    DeviceState state_{DeviceState::Booting};
    VisitorSession session_{};
    QueueHandle_t event_queue_{nullptr};
    TaskHandle_t button_monitor_task_{nullptr};
    volatile bool button_monitor_running_{false};
    std::atomic_bool visitor_capture_window_active_{false};
    std::atomic_bool shutting_down_{false};
    std::atomic_uint32_t next_press_number_{1};
    // A true value means the controller event loop can handle one repress now.
    // The button monitor atomically reserves it; all other represses retain
    // their lifecycle data but are forbidden from dispatching delayed alerts.
    std::atomic_bool repress_alert_slot_available_{false};
    std::atomic_uint32_t pending_button_events_{0};
    std::atomic_uint32_t outstanding_followup_turns_{0};
    std::uint32_t alert_cycles_started_{0};
    std::uint32_t alert_cycles_finished_{0};
    std::uint32_t uploads_succeeded_{0};
    bool wifi_rf_active_{false};
    bool cam_pwr_active_{false};
    std::int64_t last_snapshot_capture_us_{0};

    CameraService camera_;
    AudioService audio_;
    ConnectivityManager connectivity_;
    IntercomService intercom_;
    PowerManager power_;
};

}  // namespace doorbell
