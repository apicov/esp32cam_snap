#include <esp_system.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "driver/gpio.h"
#include "esp_heap_caps.h"
//#include <inttypes.h> // For PRIu32

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_log.h"

//#include "esp_event.h"
//#include "esp_netif.h"
//#include "esp_http_client.h"


//#include "mqtt_client.h"
#include "private_data.h"
#include "sd_card.h"


#include <stdint.h>//lib for ints i-e int8_t upto 1int64_t
#include <string.h> //for string data i-e char array
#include <stdbool.h> //for boolean data type
#include <stdio.h> // macros,input output, files etc
#include "nvs.h" //non volatile storage important for saving data while code runs
#include "nvs_flash.h" // storing in nvs flash memory
#include "freertos/FreeRTOS.h" //freertos for realtime opertaitons
#include "freertos/task.h" // creating a task handler and assigning priority

#include "mbedtls/base64.h"

#include "camera_ctl.h"
#include "WiFiStation.hpp" //wifi station class                                  
#include "MQTTClient.hpp" //mqtt client class

WiFiStation wifi(SSID, PASSWORD); //wifi object with ssid and password
MQTTClient mqtt(MQTT_URI); //mqtt object with broker uri
CameraCtl cam; 

static constexpr const char* TAG = "CAMERA";



QueueHandle_t gpio_evt_queue = NULL;  // FreeRTOS queue for GPIO events
QueueHandle_t camera_evt_queue = NULL;  // FreeRTOS queue for camera trigger events

//This macro explicitly places the variable in external PSRAM.
uint8_t  *img_buffer; 
void save_cam_image(char *fname, camera_fb_t *pic, uint8_t *img_buffer);


char  *b64_buffer; //buffer for base64 encoding
void base64_encode(const uint8_t *input, size_t input_len, char *output, size_t output_len); 

//static esp_mqtt_client_handle_t mqtt_client = NULL; // Global MQTT client handle


void camera_task(void *p);
void mqtt_task(void *p);


size_t b64_size;

extern "C" void app_main()
{
    ESP_LOGI(TAG, "application started");

    size_t image_size = 160 * 120 * 3;
    // Allocate for color image (RGB) in external ram
    img_buffer = (uint8_t*)heap_caps_malloc(image_size, MALLOC_CAP_SPIRAM);
    if (img_buffer == NULL) {
      printf("failed to allocate memory in psram\n");
      return;
    }
  
    // Calculate the required output buffer size for Base64 encoding
    b64_size = (4 * ((image_size + 2) / 3)) + 1;  // +1 for the null terminator

    // Allocate base64_encode  in external ram
    b64_buffer = (char*)heap_caps_malloc(b64_size, MALLOC_CAP_SPIRAM);
    if ( b64_buffer == NULL) {
        printf("Failed to allocate memory in PSRAM\n");
        return;
    }


    gpio_set_direction(GPIO_NUM_33, GPIO_MODE_OUTPUT);

    //gpio_set_direction(SWITCH_GPIO, GPIO_MODE_INPUT);
    //gpio_set_intr_type(SWITCH_GPIO, GPIO_INTR_NEGEDGE);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    camera_evt_queue = xQueueCreate(10, sizeof(uint8_t));

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    sdmmc_card_t *card;
    ret = initi_sd_card("/sdcard", &card);
    if (ret != ESP_OK) {
        ESP_LOGE("SD_CARD", "initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = cam.init_camera();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "err: %s\n", esp_err_to_name(ret));
        return;
    }

    //initialize WiFi
    wifi.init();
    ESP_LOGI(TAG, "wifi connected");

    //wait for wifi to connect
    while(!wifi.is_connected()){
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    //initialize mqtt client
    mqtt.init();

    mqtt.register_event_callback(MQTT_EVENT_CONNECTED, [](void* event_data) {
        ESP_LOGI(TAG, "MQTT_CONNECTED");
        mqtt.subscribe("/camera/cmd", 0);
        });

    ESP_LOGI(TAG, "mqtt client initialized");

    //vTaskDelay(5000 / portTICK_PERIOD_MS);

    xTaskCreate(camera_task, "camera",4096, NULL, 5, NULL);
    //xTaskCreate(mqtt_task, "mqtt",4096, NULL, 5, NULL);
    //xTaskCreate(gpio_task, "main", 4096, NULL, 5, NULL);
   //heap_caps_free(img_buffer); // Free when done
} // end of app_main   


void camera_task(void *p)
{
    
    char photo_name[50];
    unsigned int i = 0;
    uint8_t cmd;

    while(1)
    {
        //xQueueReceive(camera_evt_queue, &cmd, portMAX_DELAY);
        if(mqtt.is_connected()){
            gpio_set_level(GPIO_NUM_33, 0);
            //vTaskDelay(2500 / portTICK_PERIOD_MS);
            //gpio_set_level(GPIO_NUM_33, 0);
            //vTaskDelay(2500 / portTICK_PERIOD_MS);
            
            //sprintf(photo_name, "/sdcard/pic_%u.ppm", i++);
            cam.capture();

            //convert image to base64
            base64_encode(cam.pic->buf, cam.pic->len, b64_buffer, b64_size);
            
            //publish encoded image
            mqtt.publish("/camera/img", b64_buffer, 2, 0);
            ESP_LOGI(TAG, "image sent");

            //save_cam_image(photo_name, cam.pic, img_buffer);
            cam.free_buffer();

            gpio_set_level(GPIO_NUM_33, 1);
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
        else{
            ESP_LOGE(TAG, "MQTT not connected");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}


void save_cam_image(char *fname, camera_fb_t *pic, uint8_t *img_buffer) {

    //ESP_LOGI("Memory", "Free heap: %lu", esp_get_free_heap_size());

    if (pic->format == PIXFORMAT_JPEG) {

        fmt2rgb888(pic->buf, pic->len, PIXFORMAT_JPEG, img_buffer);

        resizeColorImage(img_buffer, 160, 120, img_buffer, 96, 96);

        saveAsPPM(fname, img_buffer, 96, 96);
    }
}



void base64_encode(const uint8_t *input, size_t input_len, char *output, size_t output_len) {
    size_t olen = 0;
    int ret = mbedtls_base64_encode((unsigned char *)output, output_len, &olen, input, input_len);
    if (ret != 0) {
        ESP_LOGE("BASE64", "Base64 encoding failed with error code: %d", ret);
    } }
