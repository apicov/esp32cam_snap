#include "image_utils.h"


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
        ESP_LOGE("PPM","Failed to open file: %s", filename);
        return;
    }

    // Write the PPM header
    fprintf(file, "P6\n%d %d\n255\n", width, height);

    // Calculate the total number of pixels
    int totalPixels = width * height;

    // Write the RGB data to the file
    if (fwrite(image, 1, totalPixels * 3, file) != totalPixels * 3) {
        ESP_LOGE("PPM","Error writing image data to file");
    }

    // Close the file
    fclose(file);

    ESP_LOGI("PPM", "PPM image saved successfully: %s", filename);
}