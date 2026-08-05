#include "services/power_manager.hpp"

#include "board_pins.h"
#include "camera_power.h"
#include "core_bringup.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace doorbell {
namespace {
constexpr const char *kTag = "PowerManager";
constexpr std::uint32_t kInputReleaseTimeoutMs = 5000;
constexpr std::uint64_t kRecoveryWakeUs = 30ULL * 1000ULL * 1000ULL;
constexpr std::uint64_t kPirWakeMask = 1ULL << static_cast<unsigned>(PIR_WAKE_PIN);
}

esp_err_t PowerManager::initialize_safe_state() const
{
    core_bringup_configure_safe_gpio_state(true);

    esp_err_t first_err = rtc_gpio_deinit(DOORBELL_BUTTON_PIN);
    const esp_err_t pir_err = rtc_gpio_deinit(PIR_WAKE_PIN);
    if (first_err == ESP_OK && pir_err != ESP_OK) {
        first_err = pir_err;
    }

    gpio_config_t inputs{};
    inputs.pin_bit_mask = (1ULL << static_cast<unsigned>(DOORBELL_BUTTON_PIN)) |
                          (1ULL << static_cast<unsigned>(PIR_WAKE_PIN));
    inputs.mode = GPIO_MODE_INPUT;
    inputs.pull_up_en = GPIO_PULLUP_DISABLE;
    inputs.pull_down_en = GPIO_PULLDOWN_DISABLE;
    inputs.intr_type = GPIO_INTR_DISABLE;
    const esp_err_t gpio_err = gpio_config(&inputs);
    if (first_err == ESP_OK && gpio_err != ESP_OK) {
        first_err = gpio_err;
    }
    return first_err;
}

WakeReason PowerManager::wake_reason() const
{
    const std::uint32_t causes = esp_sleep_get_wakeup_causes();
    if (causes == 0) {
        return WakeReason::ColdBoot;
    }
    if ((causes & (1U << ESP_SLEEP_WAKEUP_EXT0)) != 0) {
        return WakeReason::Button;
    }
    if ((causes & (1U << ESP_SLEEP_WAKEUP_EXT1)) != 0 &&
        (esp_sleep_get_ext1_wakeup_status() & kPirWakeMask) != 0) {
        return WakeReason::Motion;
    }
    if ((causes & (1U << ESP_SLEEP_WAKEUP_TIMER)) != 0) {
        return WakeReason::TimerRecovery;
    }
    return WakeReason::Other;
}

bool PowerManager::wait_for_level(int pin, int target_level,
                                  std::uint32_t timeout_ms)
{
    const TickType_t delay = pdMS_TO_TICKS(10);
    const std::uint32_t attempts = timeout_ms / 10U;
    for (std::uint32_t i = 0; i < attempts; ++i) {
        if (gpio_get_level(static_cast<gpio_num_t>(pin)) == target_level) {
            return true;
        }
        vTaskDelay(delay);
    }
    return gpio_get_level(static_cast<gpio_num_t>(pin)) == target_level;
}

esp_err_t PowerManager::configure_rtc_input(int pin)
{
    const auto gpio = static_cast<gpio_num_t>(pin);
    esp_err_t err = rtc_gpio_init(gpio);
    if (err != ESP_OK) {
        return err;
    }
    err = rtc_gpio_set_direction(gpio, RTC_GPIO_MODE_INPUT_ONLY);
    if (err != ESP_OK) {
        return err;
    }
    err = rtc_gpio_pullup_dis(gpio);
    if (err != ESP_OK) {
        return err;
    }
    return rtc_gpio_pulldown_dis(gpio);
}

[[noreturn]] void PowerManager::enter_deep_sleep() const
{
    ESP_LOGI(kTag, "Preparing safe hardware state for deep sleep");
    (void)camera_power_disable();
    core_bringup_configure_safe_gpio_state(true);
    gpio_set_level(STATUS_LED_PIN, 0);
    gpio_set_level(BUTTON_LED_PIN, 0);

    const bool button_released =
        wait_for_level(DOORBELL_BUTTON_PIN, 1, kInputReleaseTimeoutMs);
    const bool pir_idle = wait_for_level(PIR_WAKE_PIN, 0,
                                         kInputReleaseTimeoutMs);

    esp_err_t err = esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "Could not clear previous wake sources: %s",
                 esp_err_to_name(err));
    }

    bool recovery_timer_needed = false;
    bool wake_source_enabled = false;
    if (button_released) {
        err = configure_rtc_input(DOORBELL_BUTTON_PIN);
        if (err == ESP_OK) {
            err = esp_sleep_enable_ext0_wakeup(DOORBELL_BUTTON_PIN, 0);
        }
        if (err == ESP_OK) {
            wake_source_enabled = true;
            ESP_LOGI(kTag, "EXT0 button wake enabled on GPIO%d LOW",
                     DOORBELL_BUTTON_PIN);
        } else {
            recovery_timer_needed = true;
            ESP_LOGE(kTag, "Button wake setup failed: %s",
                     esp_err_to_name(err));
        }
    } else {
        recovery_timer_needed = true;
        ESP_LOGW(kTag, "Button remained LOW; suppressing immediate wake loop");
    }

    if (pir_idle) {
        err = configure_rtc_input(PIR_WAKE_PIN);
        if (err == ESP_OK) {
            err = esp_sleep_enable_ext1_wakeup_io(
                kPirWakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
        }
        if (err == ESP_OK) {
            wake_source_enabled = true;
            ESP_LOGI(kTag, "EXT1 PIR wake enabled on GPIO%d HIGH", PIR_WAKE_PIN);
        } else {
            recovery_timer_needed = true;
            ESP_LOGE(kTag, "PIR wake setup failed: %s", esp_err_to_name(err));
        }
    } else {
        recovery_timer_needed = true;
        ESP_LOGW(kTag, "PIR remained HIGH; suppressing immediate wake loop");
    }

    if (recovery_timer_needed || !wake_source_enabled) {
        err = esp_sleep_enable_timer_wakeup(kRecoveryWakeUs);
        if (err == ESP_OK) {
            wake_source_enabled = true;
            ESP_LOGI(kTag, "30-second recovery wake enabled");
        } else {
            ESP_LOGE(kTag, "Recovery timer setup failed: %s",
                     esp_err_to_name(err));
        }
    }

    if (!wake_source_enabled) {
        ESP_LOGE(kTag, "No wake source could be configured; restarting safely");
        esp_restart();
    }

    // App logging remains available after wake. Suppressing only the ROM's
    // deep-sleep banner removes avoidable latency before the early wake stub.
    esp_deep_sleep_disable_rom_logging();
    ESP_LOGI(kTag, "Entering deep sleep");
    esp_deep_sleep_start();
    __builtin_unreachable();
}

}  // namespace doorbell
