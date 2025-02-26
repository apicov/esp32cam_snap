#include <cstdint>
#include <cstring>
#include <cstdio>

// ESP-IDF
#include <esp_system.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// ESP-IDF components
#include <driver/gpio.h>
#include <mbedtls/base64.h>
#include <nvs.h> //non volatile storage important for saving data while code runs
#include <nvs_flash.h>

// ESP managed components
#include <tensorflow/lite/core/c/common.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/system_setup.h>
#include <tensorflow/lite/schema/schema_generated.h>

// Local components
#include <camera_ctl.h>
#include <sd_card.h>
#include <MQTTClient.hpp>
#include <WiFiStation.hpp>

// Application configuration
#include "private_data.h"

static const char* TAG = "APP_MAIN";

/* prototypes */
void camera_task(void *p);
void mqtt_task(void *p);
void start_mqtt_client();
void base64_encode(const uint8_t *in, size_t in_len, char *out, size_t out_len);

/* globals */
WiFiStation wifi(SSID, PASSWORD); //wifi object with ssid and password
MQTTClient mqtt(MQTT_URI); //mqtt object with broker uri

CameraCtl cam;
QueueHandle_t camera_evt_queue = NULL;  // FreeRTOS queue for camera trigger events
uint8_t  *img_buffer;

char  *b64_buffer; //buffer for base64 encoding
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

    wifi.register_event_callback( IP_EVENT, IP_EVENT_STA_GOT_IP, [](void* event_data) {
        auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "wifi connected");
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        //start mqtt client when wifi is connected
        start_mqtt_client();
        });

    xTaskCreate(camera_task, "camera",4096, NULL, 5, NULL);

    ESP_LOGI(__func__, "ESP32-cam_AI is running");
} // end of app_main


void camera_task(void *p)
{
    uint8_t cmd;

    while(1)
    {
        //wait for mqtt command
        xQueueReceive(camera_evt_queue, &cmd, portMAX_DELAY);

        if(mqtt.is_connected()){
            gpio_set_level(GPIO_NUM_33, 0);

            cam.capture();

            //convert image to base64
            base64_encode(cam.pic->buf, cam.pic->len, b64_buffer, b64_size);

            //publish encoded image
            mqtt.publish("/camera/img", b64_buffer, 2, 0);
            ESP_LOGI(TAG, "image sent");

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

void start_mqtt_client(){
    //initialize mqtt client
    mqtt.init();

    mqtt.register_event_callback(MQTT_EVENT_CONNECTED, [](esp_mqtt_event_handle_t event_data) {
        ESP_LOGI(TAG, "MQTT_CONNECTED");
        mqtt.subscribe("/camera/cmd", 0);
        });

    // Triggered when the client receives data from a subscribed topic
    mqtt.register_event_callback(MQTT_EVENT_DATA, [](esp_mqtt_event_handle_t event_data) {
        uint8_t cmd = 1; //dummy data to send to the queue
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "Received topic: %.*s", event_data->topic_len, event_data->topic);
        ESP_LOGI(TAG, "Received data: %.*s", event_data->data_len, event_data->data);

        // Check if the received message is a snap command
        if (strncmp(event_data->topic, "/camera/cmd", event_data->topic_len) == 0) {
            if (strncmp(event_data->data, "snap", event_data->data_len) == 0) {
                ESP_LOGI(TAG, "snap command received");
                xQueueSend(camera_evt_queue, &cmd, 0);
            }
        }
    });

    ESP_LOGI(TAG, "mqtt client initialized");
}

void base64_encode(const uint8_t *input, size_t input_len, char *output, size_t output_len) {
    size_t olen = 0;
    int ret = mbedtls_base64_encode((unsigned char *)output, output_len, &olen, input, input_len);
    if (ret != 0) {
        ESP_LOGE("BASE64", "Base64 encoding failed with error code: %d", ret);
    } }
