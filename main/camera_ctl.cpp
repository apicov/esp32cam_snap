#include "camera_ctl.h"

//uint8_t img[96*96];
//uint8_t img_color[96 * 96 * 3]; // Allocate for color image (RGB)
uint8_t img_color[160 * 120 * 3]; // Allocate for color image (RGB)



void resizeColorImage(uint8_t *src, int srcWidth, int srcHeight, 
                      uint8_t *dst, int dstWidth, int dstHeight) {
    for (int y = 0; y < dstHeight; y++) {
        for (int x = 0; x < dstWidth; x++) {
            // Map destination coordinates to source coordinates
            int srcX = x * srcWidth / dstWidth;
            int srcY = y * srcHeight / dstHeight;

            // Calculate source index in 24-bit array (RGB888)
            int srcIndex = (srcY * srcWidth + srcX) * 3; // RGB888 = 3 bytes per pixel

            // Extract RGB components
            // swap r and b channels (because of bug in fmt2rgb888 function)
            uint8_t b = src[srcIndex];
            uint8_t g = src[srcIndex + 1];
            uint8_t r = src[srcIndex + 2];

            // Write to the destination array
            int dstIndex = (y * dstWidth + x) * 3; // RGB888 = 3 bytes per pixel
            dst[dstIndex] = r;
            dst[dstIndex + 1] = g;
            dst[dstIndex + 2] = b;
        }
    }
}


// Function to save an RGB888 image as a PPM file
void saveAsPPM(const char *filename, uint8_t *image, int width, int height) {
    // Open the file in binary write mode
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return;
    }

    // Write the PPM header
    fprintf(file, "P6\n%d %d\n255\n", width, height);

    // Calculate the total number of pixels
    int totalPixels = width * height;

    // Write the RGB data to the file
    if (fwrite(image, 1, totalPixels * 3, file) != totalPixels * 3) {
        printf("Error writing image data to file\n");
    }

    // Close the file
    fclose(file);

    printf("PPM image saved successfully: %s\n", filename);
}



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


void CameraCtl::capture(void *arg)
{    
    ESP_LOGI(CAM_TAG, "Starting Taking Picture!");

    camera_fb_t *pic = esp_camera_fb_get();
    
    esp_camera_fb_return(pic);
    //vTaskDelete(NULL);
    ESP_LOGI(CAM_TAG, "Finished Taking Picture!");
}


/*
void CameraCtl::capture_to_file(char *fname)
{
    ESP_LOGI(CAM_TAG, "Starting Taking Picture!");


    pic = esp_camera_fb_get();

    resizeGrayImage(pic->buf,
                    160, //srcWidth
                    120, //srcHeight 
                    img,  
                    96, //dstWidth 
                    96 );  //dstHeight


    if (pic->format == PIXFORMAT_GRAYSCALE) 
    {
        FILE *file = fopen(fname, "w");
        if (file) 
        {
            //writeBMPHeader(file, pic->width, pic->height);
            writeBMPHeader(file, 96, 96);


            for (int y = 96 - 1; y >= 0; y--) {
                fwrite(img + (y * 96), 1, 96, file);
            }

            // Write pixel data (bottom-to-top for BMP format)
            //for (int y = pic->height - 1; y >= 0; y--) {
                //fwrite(pic->buf + (y * pic->width), 1, pic->width, file);
           // }

            fclose(file);

        
            ESP_LOGI(CAM_TAG,"Image saved as BMP");
        } 
        else 
        {
            ESP_LOGI(CAM_TAG,"Failed to open file");
        }
    } 
    else 
    {
        ESP_LOGI(CAM_TAG,"Image format is not grayscale");
    }
    
    esp_camera_fb_return(pic);
    //vTaskDelete(NULL);
    ESP_LOGI(CAM_TAG, "Finished Taking Picture!");
}
*/


void CameraCtl::capture_to_file(char *fname) {
    ESP_LOGI(CAM_TAG, "Starting Taking Picture!");

    camera_fb_t *pic = esp_camera_fb_get();

    ESP_LOGI("Memory", "Free heap: %lu", esp_get_free_heap_size());

    if (pic->format == PIXFORMAT_JPEG) {

        fmt2rgb888(pic->buf, pic->len, PIXFORMAT_JPEG, img_color);

        resizeColorImage(img_color, 160, 120, img_color, 96, 96);

        saveAsPPM(fname, img_color, 96, 96);
    


        /*
        FILE *file = fopen(fname, "w");
        if (file) {

            

            fclose(file);
            ESP_LOGI(CAM_TAG, "Image saved as PPM");
        } else {
            ESP_LOGI(CAM_TAG, "Failed to open file");
            esp_camera_fb_return(pic);
        }*/
    }

    esp_camera_fb_return(pic);
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
