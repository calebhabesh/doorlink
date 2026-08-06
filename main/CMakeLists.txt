idf_component_register(SRCS "app_main.cpp" "doorbell_controller.cpp"
                    "services/camera_service.cpp" "services/connectivity_manager.cpp" "services/power_manager.cpp"
                    "wake_sleep_bringup.cpp" "wake_stub.c" "ring_fade.c"
                    "core_bringup.c" "battery_bringup.c" "battery_camera_upload_bringup.c"
                    "camera_capture.c" "camera_power.c" "camera_power_gate_bringup.c"
                    "mic_bringup.c" "speaker_bringup.c" "button_bringup.c" "pir_bringup.c"
                    "wifi_bringup.c" "camera_bringup.c" "ov5640_mode_fix.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_wifi esp_http_client esp_driver_gpio esp_driver_i2s esp_driver_ledc esp_event esp_timer esp_psram spi_flash mbedtls esp_adc esp_app_format)
