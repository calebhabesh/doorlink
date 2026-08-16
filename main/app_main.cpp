#include "battery_bringup.h"
#include "battery_camera_upload_bringup.h"
#include "button_bringup.h"
#include "camera_bringup.h"
#include "camera_power_gate_bringup.h"
#include "core_bringup.h"
#include "doorbell_controller.hpp"
#include "mic_bringup.h"
#include "pir_bringup.h"
#include "speaker_bringup.h"
#include "wake_sleep_bringup.h"
#include "wifi_bringup.h"

#include <cstdlib>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef SMART_DOORBELL_PRODUCTION_APP
#ifdef CONFIG_SMART_DOORBELL_PRODUCTION_APP
#define SMART_DOORBELL_PRODUCTION_APP CONFIG_SMART_DOORBELL_PRODUCTION_APP
#else
#define SMART_DOORBELL_PRODUCTION_APP 0
#endif
#endif

#ifndef SMART_DOORBELL_WAKE_SLEEP_DIAGNOSTIC
#ifdef CONFIG_SMART_DOORBELL_WAKE_SLEEP_DIAGNOSTIC
#define SMART_DOORBELL_WAKE_SLEEP_DIAGNOSTIC CONFIG_SMART_DOORBELL_WAKE_SLEEP_DIAGNOSTIC
#else
#define SMART_DOORBELL_WAKE_SLEEP_DIAGNOSTIC 0
#endif
#endif

#ifndef SMART_DOORBELL_MIC_BRINGUP
#ifdef CONFIG_SMART_DOORBELL_MIC_DIAGNOSTIC
#define SMART_DOORBELL_MIC_BRINGUP CONFIG_SMART_DOORBELL_MIC_DIAGNOSTIC
#else
#define SMART_DOORBELL_MIC_BRINGUP 0
#endif
#endif

#ifndef SMART_DOORBELL_CORE_BRINGUP
#define SMART_DOORBELL_CORE_BRINGUP \
    (!SMART_DOORBELL_PRODUCTION_APP && !SMART_DOORBELL_WAKE_SLEEP_DIAGNOSTIC && \
     !SMART_DOORBELL_MIC_BRINGUP)
#endif

#ifndef SMART_DOORBELL_BATTERY_BRINGUP
#define SMART_DOORBELL_BATTERY_BRINGUP 0
#endif

#ifndef SMART_DOORBELL_BATTERY_CAMERA_UPLOAD_BRINGUP
#define SMART_DOORBELL_BATTERY_CAMERA_UPLOAD_BRINGUP 0
#endif

#ifndef SMART_DOORBELL_CAMERA_ATTACHED_IDLE
#define SMART_DOORBELL_CAMERA_ATTACHED_IDLE 0
#endif

#ifndef SMART_DOORBELL_CAMERA_BRINGUP
#define SMART_DOORBELL_CAMERA_BRINGUP 0
#endif

#ifndef SMART_DOORBELL_CAMERA_POWER_GATE_BRINGUP
#define SMART_DOORBELL_CAMERA_POWER_GATE_BRINGUP 0
#endif

#ifndef SMART_DOORBELL_WIFI_BRINGUP
#define SMART_DOORBELL_WIFI_BRINGUP 0
#endif

#ifndef SMART_DOORBELL_SPEAKER_BRINGUP
#define SMART_DOORBELL_SPEAKER_BRINGUP 0
#endif

#ifndef SMART_DOORBELL_BUTTON_BRINGUP
#define SMART_DOORBELL_BUTTON_BRINGUP 0
#endif

#ifndef SMART_DOORBELL_PIR_BRINGUP
#define SMART_DOORBELL_PIR_BRINGUP 0
#endif

namespace {
constexpr const char *kTag = "app_main";
constexpr uint32_t kProductionControllerStackBytes = 16 * 1024;
constexpr UBaseType_t kProductionControllerPriority = 5;

const char *reset_reason_name(esp_reset_reason_t reason)
{
    switch (reason) {
        case ESP_RST_POWERON: return "power-on";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt-watchdog";
        case ESP_RST_TASK_WDT: return "task-watchdog";
        case ESP_RST_WDT: return "watchdog";
        case ESP_RST_DEEPSLEEP: return "deep-sleep-wake";
        case ESP_RST_BROWNOUT: return "brownout";
        default: return "other";
    }
}

void production_controller_task(void *arg)
{
    auto *controller = static_cast<doorbell::DoorbellController *>(arg);
    controller->run();
}

void start_production_controller()
{
    // The controller and its member work buffers use static storage. Its call
    // chain includes camera, HTTP multipart upload, MQTT, and session handling,
    // so do not run it on ESP-IDF's small CONFIG_ESP_MAIN_TASK_STACK_SIZE stack.
    static doorbell::DoorbellController controller;
    const BaseType_t result = xTaskCreate(
        production_controller_task, "doorbell_ctrl",
        kProductionControllerStackBytes, &controller,
        kProductionControllerPriority, nullptr);
    if (result != pdPASS) {
        ESP_LOGE(kTag, "Failed to create production controller task");
        std::abort();
    }
}
}  // namespace

extern "C" void app_main(void)
{
    const esp_reset_reason_t reset_reason = esp_reset_reason();
    ESP_LOGI(kTag, "Reset reason=%s (%d)", reset_reason_name(reset_reason),
             static_cast<int>(reset_reason));
#if SMART_DOORBELL_WAKE_SLEEP_DIAGNOSTIC
    run_wake_sleep_bringup();
#elif SMART_DOORBELL_PRODUCTION_APP
    start_production_controller();
#elif SMART_DOORBELL_MIC_BRINGUP
    run_mic_bringup();
#elif SMART_DOORBELL_CORE_BRINGUP
    run_core_bringup(SMART_DOORBELL_CAMERA_ATTACHED_IDLE);
#elif SMART_DOORBELL_BATTERY_BRINGUP
    run_battery_bringup();
#elif SMART_DOORBELL_BATTERY_CAMERA_UPLOAD_BRINGUP
    run_battery_camera_upload_bringup();
#elif SMART_DOORBELL_CAMERA_POWER_GATE_BRINGUP
    run_camera_power_gate_bringup();
#elif SMART_DOORBELL_CAMERA_BRINGUP
    run_camera_bringup();
#elif SMART_DOORBELL_WIFI_BRINGUP
    run_wifi_bringup();
#elif SMART_DOORBELL_SPEAKER_BRINGUP
    run_speaker_bringup();
#elif SMART_DOORBELL_BUTTON_BRINGUP
    run_button_bringup();
#elif SMART_DOORBELL_PIR_BRINGUP
    run_pir_bringup();
#else
    // Keep the production fallback consistent with the configured path above.
    start_production_controller();
#endif
}
