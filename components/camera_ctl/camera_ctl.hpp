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
     * @brief Representation of a picture issued by CameraCtl.
     *
     */
    struct Picture
    {
        /* XXX: Pictures can only be taken through the CameraCtl,
         * because only one picture can be taken at the time, if
         * multiple instance are created then problems with the framebuffer
         * may occur. Probably one solution to this, is to release the
         * framebuffer every time a new instance is created and document
         * this behavior, and let the user to assume the responsability.
         */
        friend CameraCtl;

        /**
         * @brief: Returns a non-modifiable pointer to the picture's buffer.
         *
         */
        const uint8_t* image() const;
        /**
         * @brief: return the size of the picture's buffer.
         *
         */
        size_t size() const;

    private:
        Picture();
        ~Picture();
        camera_fb_t *fb;
    };

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
    void capture_do(std::function<void(const Picture &)>);
private:
    esp_err_t camera_xclk_init(uint32_t freq_hz);
};
