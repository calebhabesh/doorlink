#pragma once

#include <cstddef>
#include <cstdint>

#include "camera_capture.h"
#include "esp_err.h"

namespace doorbell {

class CapturedImage final {
public:
    CapturedImage() = default;
    ~CapturedImage();

    CapturedImage(const CapturedImage &) = delete;
    CapturedImage &operator=(const CapturedImage &) = delete;
    CapturedImage(CapturedImage &&) = delete;
    CapturedImage &operator=(CapturedImage &&) = delete;

    const std::uint8_t *data() const { return image_.data; }
    std::size_t size() const { return image_.size; }
    std::uint16_t width() const { return image_.width; }
    std::uint16_t height() const { return image_.height; }
    bool valid() const { return image_.data != nullptr && image_.size != 0; }
    void reset();

private:
    friend class CameraService;
    camera_owned_jpeg_t image_{};
};

class CameraService final {
public:
    esp_err_t capture(CapturedImage &image, int timeout_ms = 4000) const;
};

}  // namespace doorbell
