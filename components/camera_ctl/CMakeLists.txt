idf_component_register(SRCS "camera_ctl.cpp" "image_utils.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES "esp32-camera"
                       PRIV_REQUIRES "esp_driver_gpio" "esp_driver_i2c")
