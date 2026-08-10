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

extern "C" void app_main(void)
{
#if SMART_DOORBELL_WAKE_SLEEP_DIAGNOSTIC
    run_wake_sleep_bringup();
#elif SMART_DOORBELL_PRODUCTION_APP
    // The controller owns the audio work buffers and lives until deep sleep.
    // Keep it out of the small ESP-IDF main-task stack.
    static doorbell::DoorbellController controller;
    controller.run();
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
    static doorbell::DoorbellController controller;
    controller.run();
#endif
}
