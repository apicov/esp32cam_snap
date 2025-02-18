#include "camera_ctl.h"

//uint8_t img[96*96];
//uint8_t img_color[96 * 96 * 3]; // Allocate for color image (RGB)

esp_err_t CameraCtl::init_camera(void)
{
    camera_config_t camera_config = {
    CAM_PIN_PWDN,        // pin_pwdn
    CAM_PIN_RESET,       // pin_reset
    CAM_PIN_XCLK,        // pin_xclk
    CAM_PIN_SIOD,        // pin_sccb_sda
    CAM_PIN_SIOC,        // pin_sccb_scl
    CAM_PIN_D7,          // pin_d7
    CAM_PIN_D6,          // pin_d6
    CAM_PIN_D5,          // pin_d5
    CAM_PIN_D4,          // pin_d4
    CAM_PIN_D3,          // pin_d3
    CAM_PIN_D2,          // pin_d2
    CAM_PIN_D1,          // pin_d1
    CAM_PIN_D0,          // pin_d0
    CAM_PIN_VSYNC,       // pin_vsync
    CAM_PIN_HREF,        // pin_href
    CAM_PIN_PCLK,        // pin_pclk
    CONFIG_XCLK_FREQ,    // xclk_freq_hz
    LEDC_TIMER_0,        // ledc_timer
    LEDC_CHANNEL_0,      // ledc_channel
    PIXFORMAT_JPEG,//PIXFORMAT_GRAYSCALE, //PIXFORMAT_JPEG,      // pixel_format
    FRAMESIZE_QQVGA,      // frame_size
    12,                   // jpeg_quality
    1,                    // fb_count
    CAMERA_FB_IN_PSRAM,   // fb_location (or CAMERA_FB_IN_DRAM)
    CAMERA_GRAB_WHEN_EMPTY, // grab_mode
    I2C_NUM_0 // sccb_i2c_port
    };


    esp_err_t err = camera_xclk_init(20'000'000);
    if (err != ESP_OK)
    {
        return err;
    }

    err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        return err;
    }
    return ESP_OK;
}


void CameraCtl::capture(void)
{    
    ESP_LOGI(CAM_TAG, "Starting Taking Picture!");

    pic = esp_camera_fb_get();
    
    ESP_LOGI(CAM_TAG, "Finished Taking Picture!");
}



void CameraCtl::free_buffer()
{
    esp_camera_fb_return(pic);
}


esp_err_t CameraCtl::camera_xclk_init(uint32_t freq_hz) {

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE, // High-speed mode
        .duty_resolution = LEDC_TIMER_1_BIT, // Minimal duty resolution for clock
        .timer_num = LEDC_TIMER_0,         // Use LEDC_TIMER_0
        .freq_hz = freq_hz,                // Set the desired frequency
        .clk_cfg = LEDC_AUTO_CLK           // Automatically select clock source
    };

    // Configure the LEDC timer
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE("camera_xclk", "LEDC timer config failed, rc=%d", err);
        return err;
    }

    // Configure the LEDC channel for XCLK pin
    ledc_channel_config_t ledc_channel = {
        .gpio_num = CAM_PIN_XCLK, // Replace with your XCLK GPIO number
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 1, // Minimal duty cycle for clock generation
        .hpoint = 0
    };

    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE("camera_xclk", "LEDC channel config failed, rc=%d", err);
        return err;
    }

    return ESP_OK;
}
