#include "services/camera_service.hpp"

#include "esp_log.h"

namespace doorbell {
namespace {
constexpr const char *kTag = "CameraService";
}

CapturedImage::~CapturedImage()
{
    reset();
}

void CapturedImage::reset()
{
    camera_capture_release(&image_);
}

esp_err_t CameraService::capture(CapturedImage &image) const
{
    image.reset();
    const esp_err_t err = camera_capture_qxga_owned(&image.image_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "QXGA capture failed: %s", esp_err_to_name(err));
    }
    return err;
}

}  // namespace doorbell
