#include "image_utils.h"


void resizeColorImage(uint8_t *src, int srcWidth, int srcHeight, 
                      uint8_t *dst, int dstWidth, int dstHeight) {
    for (int y = 0; y < dstHeight; y++) {
        for (int x = 0; x < dstWidth; x++) {
            // Map destination coordinates to source coordinates
            int srcX = x * srcWidth / dstWidth;
            int srcY = y * srcHeight / dstHeight;

            // Calculate source and destination indices
            int srcIndex = (srcY * srcWidth + srcX) * 3;
            int dstIndex = (y * dstWidth + x) * 3;

            // Copy as-is (no BGR/RGB swap)
            dst[dstIndex] = src[srcIndex];
            dst[dstIndex + 1] = src[srcIndex + 1];
            dst[dstIndex + 2] = src[srcIndex + 2];
        }
    }
}


void resizeColorImageBGRtoRGB(uint8_t *src, int srcWidth, int srcHeight,
                              uint8_t *dst, int dstWidth, int dstHeight) {
    for (int y = 0; y < dstHeight; y++) {
        for (int x = 0; x < dstWidth; x++) {
            // Map destination coordinates to source coordinates
            int srcX = x * srcWidth / dstWidth;
            int srcY = y * srcHeight / dstHeight;

            // Calculate source and destination pixel positions (3 bytes per pixel)
            int srcPos = (srcY * srcWidth + srcX) * 3;
            int dstPos = (y * dstWidth + x) * 3;

            // Resize and swap BGR to RGB in one pass
            dst[dstPos] = src[srcPos + 2];     // R = src B
            dst[dstPos + 1] = src[srcPos + 1]; // G = src G
            dst[dstPos + 2] = src[srcPos];     // B = src R
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