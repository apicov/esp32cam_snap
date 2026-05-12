idf_component_register(SRCS "esp32cam_snap.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES "mqtt_client" "esp_psram" "esp_wifi" "nvs_flash" "camera_ctl" "esp_driver_gpio" "wifi_station")
