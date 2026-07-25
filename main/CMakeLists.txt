idf_component_register(SRCS "main.c" "core_bringup.c" "camera_power.c" "mic_bringup.c" "speaker_bringup.c" "button_bringup.c" "pir_bringup.c" "wifi_bringup.c" "camera_bringup.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_wifi esp_http_client esp_driver_gpio esp_driver_i2s esp_event esp_psram spi_flash)
