#include "wake_sleep_bringup.h"

#include <cstdint>

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "services/power_manager.hpp"
#include "ring_fade.h"
#include "wake_stub.h"

namespace {
constexpr const char *kTag = "wake_sleep_bringup";
RTC_DATA_ATTR std::uint32_t s_wake_count = 0;

const char *wake_name(doorbell::WakeReason reason)
{
    switch (reason) {
    case doorbell::WakeReason::ColdBoot:
        return "COLD_BOOT";
    case doorbell::WakeReason::Button:
        return "BUTTON_EXT0";
    case doorbell::WakeReason::Motion:
        return "PIR_EXT1";
    case doorbell::WakeReason::TimerRecovery:
        return "TIMER_RECOVERY";
    case doorbell::WakeReason::Other:
        return "OTHER";
    }
    return "UNKNOWN";
}

void set_leds(int level)
{
    gpio_set_level(STATUS_LED_PIN, level);
    gpio_set_level(BUTTON_LED_PIN, level);
}

void pulse_leds(unsigned count, std::uint32_t on_ms, std::uint32_t off_ms)
{
    for (unsigned i = 0; i < count; ++i) {
        set_leds(1);
        vTaskDelay(pdMS_TO_TICKS(on_ms));
        set_leds(0);
        vTaskDelay(pdMS_TO_TICKS(off_ms));
    }
}
}  // namespace

extern "C" void run_wake_sleep_bringup(void)
{
    wake_stub_install();

    doorbell::PowerManager power;
    const esp_err_t err = power.initialize_safe_state();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Safe-state initialization failed: %s",
                 esp_err_to_name(err));
        pulse_leds(10, 75, 75);
        power.enter_deep_sleep();
    }

    const doorbell::WakeReason reason = power.wake_reason();

    // Acknowledge a real wake before emitting logs or doing other diagnostic
    // work. Safe-state initialization above establishes the LED GPIO modes.
    if (reason == doorbell::WakeReason::Button) {
        set_leds(1);
    }

    ESP_LOGW(kTag, "WAKE/SLEEP DIAGNOSTIC: reason=%s retained_wakes=%u",
             wake_name(reason), static_cast<unsigned>(s_wake_count));
    ESP_LOGI(kTag,
             "Camera, Wi-Fi, MQTT, amplifier, I2S, audio, and upload are disabled");

    if (reason == doorbell::WakeReason::Button) {
        ++s_wake_count;
        ESP_LOGI(kTag, "EXT0 BUTTON WAKE PASS: count=%u",
                 static_cast<unsigned>(s_wake_count));
        const esp_err_t fade_err = ring_fade_start(1800);
        if (fade_err != ESP_OK) {
            ESP_LOGE(kTag, "GPIO48 fade start failed: %s",
                     esp_err_to_name(fade_err));
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
        ring_fade_stop();
        gpio_set_level(STATUS_LED_PIN, 0);
    } else if (reason == doorbell::WakeReason::ColdBoot) {
        ESP_LOGI(kTag, "Cold boot: arming GPIO2 EXT0 and GPIO3 EXT1 wake");
        pulse_leds(2, 200, 200);
    } else if (reason == doorbell::WakeReason::Motion) {
        ++s_wake_count;
        ESP_LOGI(kTag, "EXT1 PIR WAKE PASS: count=%u",
                 static_cast<unsigned>(s_wake_count));
        pulse_leds(3, 300, 150);
    } else {
        ESP_LOGW(kTag, "Non-event wake; returning to sleep without peripherals");
        pulse_leds(1, 100, 100);
    }

    ESP_LOGI(kTag, "Diagnostic complete; waiting for inactive inputs then sleeping");
    ring_fade_stop();
    power.enter_deep_sleep();
}
