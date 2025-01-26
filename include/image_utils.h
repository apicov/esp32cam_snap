#pragma once
#include <stdint.h>//lib for ints i-e int8_t upto 1int64_t
#include <stdio.h> // macros,input output, files etc
#include <esp_log.h>


void resizeColorImage(uint8_t *src, int srcWidth, int srcHeight, 
                      uint8_t *dst, int dstWidth, int dstHeight);

void saveAsPPM(const char *filename, uint8_t *image, int width, int height);