// ESP
#include <esp_log.h>
#include <esp_check.h>

// ESP-IDF
#include <driver/gpio.h>
#include <driver/i2c.h>

#include "camera_ctl.hpp"

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
#define CAM_FLASH_LAMP  GPIO_NUM_4

#define CONFIG_XCLK_FREQ 20'000'000
#define CONFIG_OV2640_SUPPORT 1
#define CONFIG_OV7725_SUPPORT 1
#define CONFIG_OV3660_SUPPORT 1
#define CONFIG_OV5640_SUPPORT 1

CameraCtl::CameraCtl()
{
    camera_config_t config = {
      .pin_pwdn = CAM_PIN_PWDN,
      .pin_reset = CAM_PIN_RESET,
      .pin_xclk = CAM_PIN_XCLK,
      .pin_sccb_sda = CAM_PIN_SIOD,
      .pin_sccb_scl = CAM_PIN_SIOC,
      .pin_d7 = CAM_PIN_D7,
      .pin_d6 = CAM_PIN_D6,
      .pin_d5 = CAM_PIN_D5,
      .pin_d4 = CAM_PIN_D4,
      .pin_d3 = CAM_PIN_D3,
      .pin_d2 = CAM_PIN_D2,
      .pin_d1 = CAM_PIN_D1,
      .pin_d0 = CAM_PIN_D0,
      .pin_vsync = CAM_PIN_VSYNC,
      .pin_href  = CAM_PIN_HREF,
      .pin_pclk  = CAM_PIN_PCLK,

      .xclk_freq_hz = CONFIG_XCLK_FREQ,
      .ledc_timer = LEDC_TIMER_0,
      .ledc_channel = LEDC_CHANNEL_0,

      .pixel_format = PIXFORMAT_JPEG,
      .frame_size = FRAMESIZE_QQVGA,

      .jpeg_quality = 12,
      .fb_count = 1,
      .fb_location = CAMERA_FB_IN_PSRAM,
      .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
      .sccb_i2c_port = I2C_NUM_0
    };

    /* XXX: for now abort if the camera couldn't be initialized,
     * but maybe is best to allow the user to do it instead
     */
    ESP_ERROR_CHECK(esp_camera_init(&config));
    gpio_set_direction(CAM_FLASH_LAMP, GPIO_MODE_OUTPUT);
    ESP_LOGD(TAG, "Camera initialized");
}


void CameraCtl::capture_do(std::function<void(const Picture &)> f)
{
    gpio_set_level(CAM_FLASH_LAMP, 1);
    Picture p{};
    gpio_set_level(CAM_FLASH_LAMP, 0);
    return f(p);
}


esp_err_t CameraCtl::camera_xclk_init(uint32_t freq_hz) {

    // Configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,  // High-speed mode
        .duty_resolution = LEDC_TIMER_1_BIT, // Minimal duty resolution for clock
        .timer_num = LEDC_TIMER_0,           // Use LEDC_TIMER_0
        .freq_hz = freq_hz,                  // Set the desired frequency
        .clk_cfg = LEDC_AUTO_CLK,            // Automatically select clock source
        .deconfigure = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&ledc_timer), TAG, "ledc_timer");

    // Configure the LEDC channel for XCLK pin
    ledc_channel_config_t ledc_channel = {
        .gpio_num = CAM_PIN_XCLK, // Replace with your XCLK GPIO number
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,               // TODO: default
        .timer_sel = LEDC_TIMER_0,
        .duty = 1, // Minimal duty cycle for clock generation
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD, // TODO: default
        .flags = { .output_invert = 1 },              // TODO: default
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ledc_channel), TAG, "ledc_channel");

    return ESP_OK;
}

/* CameraCtl::Picture */
/* ================== */
CameraCtl::Picture::Picture() : fb{esp_camera_fb_get()}
{
    ESP_LOGI(TAG, "Snapshot taken");
}

CameraCtl::Picture::~Picture()
{
    ESP_LOGD(TAG, "Release the snapshot's framebuffer");
    esp_camera_fb_return(fb);
}

const uint8_t *CameraCtl::Picture::image() const
{
    return fb->buf;
}

size_t CameraCtl::Picture::size() const
{
    return fb->len;
}
