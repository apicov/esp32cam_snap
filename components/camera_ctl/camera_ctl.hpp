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
    constexpr static const char* TAG = "camera_ctl";
    /**
     * @brief initialize the camera
     *
     * @note must be called before using the object
     */
    // TODO: it might be better to make call this as part of
    // the defaul constructor.
    esp_err_t init_camera(void);

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
