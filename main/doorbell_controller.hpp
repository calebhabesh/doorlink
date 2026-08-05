#pragma once

#include <cstdint>

#include "esp_err.h"
#include "services/camera_service.hpp"
#include "services/connectivity_manager.hpp"
#include "services/power_manager.hpp"

namespace doorbell {

enum class DeviceState : std::uint8_t {
    Booting,
    IdlePreparation,
    Capturing,
    Connecting,
    Triggering,
    Disconnecting,
    Uploading,
    ShuttingDown,
};

class DoorbellController final {
public:
    [[noreturn]] void run();

private:
    void set_state(DeviceState state);
    esp_err_t capture_with_retry(CapturedImage &image);
    esp_err_t trigger_with_retry(const char *event_id,
                                 const char *event_type,
                                 const char *device_id,
                                 const char *firmware_version);
    esp_err_t upload_with_retry(const CapturedImage &image,
                                const char *event_type,
                                const char *event_id,
                                const char *device_id,
                                const char *firmware_version);
    [[noreturn]] void sleep();

    DeviceState state_{DeviceState::Booting};
    CameraService camera_;
    ConnectivityManager connectivity_;
    PowerManager power_;
};

}  // namespace doorbell
