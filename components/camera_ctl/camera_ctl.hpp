#pragma once

#include <functional>

#include "esp_camera.h"
#include "esp_err.h"

#define I2C_MASTER_FREQ_HZ 100000        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0      /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0

#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22


#define CONFIG_XCLK_FREQ 20'000'000
#define CONFIG_OV2640_SUPPORT 1
#define CONFIG_OV7725_SUPPORT 1
#define CONFIG_OV3660_SUPPORT 1
#define CONFIG_OV5640_SUPPORT 1

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
