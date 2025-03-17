#pragma once

#include <functional>

#include "esp_camera.h"
#include "esp_err.h"

/**
 * @brief Thin wrapper around "esp32_camera" to simplify its usage
 *
 */
class CameraCtl
{
public:
    /**
     * @brief Tag descriptor of the class, useful for logging.
     *
     */
    static constexpr const char* TAG = "camera_ctl";

    /**
     * @brief Build a CameraCtl object
     *
     * @note It cannot be instantiated before "app_main" is called.
     *
     */
    CameraCtl();

    /**
     * @brief Capture an image and may "do" something with it
     *
     * @param f is a "FunctionObject" that takes an image as an argument and returns "void".
     *
     */
    void capture_do(std::function<void(const camera_fb_t*)>);
private:
    esp_err_t camera_xclk_init(uint32_t freq_hz);
    camera_fb_t *pic;
};
