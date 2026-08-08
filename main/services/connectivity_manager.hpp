#pragma once

#include "esp_err.h"
#include "services/camera_service.hpp"

namespace doorbell {

class ConnectivityManager final {
public:
    ConnectivityManager() = default;
    ~ConnectivityManager();

    ConnectivityManager(const ConnectivityManager &) = delete;
    ConnectivityManager &operator=(const ConnectivityManager &) = delete;

    esp_err_t connect(int timeout_ms = 0);
    esp_err_t trigger(const char *event_id, const char *event_type,
                      const char *device_id,
                      const char *firmware_version,
                      int timeout_ms = 0) const;
    esp_err_t upload(const CapturedImage &image, const char *event_type,
                     const char *event_id, const char *device_id,
                     const char *firmware_version,
                     int timeout_ms = 0) const;
    esp_err_t shutdown();
    bool connected() const { return connected_; }

private:
    bool connected_{false};
};

}  // namespace doorbell
