#include "services/connectivity_manager.hpp"

#include "esp_log.h"
#include "wifi_bringup.h"

namespace doorbell {
namespace {
constexpr const char *kTag = "ConnectivityManager";
}

ConnectivityManager::~ConnectivityManager()
{
    (void)shutdown();
}

esp_err_t ConnectivityManager::connect(int timeout_ms)
{
    if (connected_.load()) {
        return ESP_OK;
    }

    const esp_err_t err = wifi_bringup_connect_bounded_timeout(timeout_ms);
    connected_.store(err == ESP_OK);
    if (!connected_.load()) {
        ESP_LOGE(kTag, "Wi-Fi connection failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t ConnectivityManager::trigger(const char *event_id,
                                       const char *event_type,
                                       const char *device_id,
                                       const char *firmware_version,
                                       bool dispatch_alerts,
                                       int timeout_ms) const
{
    if (!connected_.load()) {
        return ESP_ERR_INVALID_STATE;
    }
    return wifi_bringup_trigger_event_timeout(event_id, event_type, device_id,
                                      firmware_version,
                                      battery_millivolts_.load(),
                                      dispatch_alerts, timeout_ms);
}

esp_err_t ConnectivityManager::upload(const CapturedImage *image,
                                      const RecordedAudio *audio,
                                      const char *event_type,
                                      const char *event_id,
                                      const char *device_id,
                                      const char *firmware_version,
                                      int timeout_ms) const
{
    if (!connected_.load() || !event_type ||
        ((!image || !image->valid()) && (!audio || !audio->valid()))) {
        return ESP_ERR_INVALID_STATE;
    }
    return wifi_bringup_upload_event_timeout(
        image && image->valid() ? image->data() : nullptr,
        image && image->valid() ? image->size() : 0,
        audio && audio->valid() ? audio->data() : nullptr,
        audio && audio->valid() ? audio->size() : 0, event_type, event_id,
        device_id, firmware_version, battery_millivolts_.load(), timeout_ms);
}

esp_err_t ConnectivityManager::shutdown()
{
    if (!connected_.load()) {
        return ESP_OK;
    }
    const esp_err_t err = wifi_bringup_stop_bounded();
    connected_.store(false);
    return err;
}

esp_err_t ConnectivityManager::close_session(const char *session_id,
                                              int timeout_ms) const
{
    if (!connected_.load()) return ESP_ERR_INVALID_STATE;
    return wifi_bringup_close_session(session_id, timeout_ms);
}

esp_err_t ConnectivityManager::complete_press(const char *press_id,
                                               std::uint32_t duration_ms,
                                               int timeout_ms) const
{
    if (!connected_.load()) return ESP_ERR_INVALID_STATE;
    return wifi_bringup_complete_press(press_id, duration_ms, timeout_ms);
}

}  // namespace doorbell
