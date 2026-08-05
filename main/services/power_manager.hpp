#pragma once

#include <cstdint>

#include "esp_err.h"

namespace doorbell {

enum class WakeReason : std::uint8_t {
    ColdBoot,
    Button,
    Motion,
    TimerRecovery,
    Other,
};

class PowerManager final {
public:
    esp_err_t initialize_safe_state() const;
    WakeReason wake_reason() const;
    [[noreturn]] void enter_deep_sleep() const;

private:
    static bool wait_for_level(int pin, int target_level,
                               std::uint32_t timeout_ms);
    static esp_err_t configure_rtc_input(int pin);
};

}  // namespace doorbell
