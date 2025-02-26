#include <cstdint>
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
#include <MQTTClient.hpp>
#include <WiFiStation.hpp>

// Application configuration
#include "private_data.h"

/* prototypes */
void camera_task(void *p);
void mqtt_task(void *p);
void start_mqtt_client();
void base64_encode(const uint8_t *in, size_t in_len, char *out, size_t out_len);

/* globals */
WiFiStation wifi(SSID, PASSWORD);
MQTTClient mqtt(MQTT_URI); //MQTT object with broker uri

CameraCtl cam;
QueueHandle_t camera_evt_queue = NULL;  // FreeRTOS queue for camera trigger events

char  *b64_buffer; //buffer for base64 encoding
size_t b64_size;

extern "C" void app_main()
{
    ESP_LOGI(__func__, "starting...");

    // TODO: if the image resolution can be adjusted, then it might be
    // a good idea to make the dimensions configurable
    size_t image_size = 160 * 120 * 3;

    // Calculate the required output buffer size for Base64 encoding
    b64_size = (4 * ((image_size + 2) / 3)) + 1;  // +1 for the null terminator

    // Allocate base64_encode in external ram
    // TODO: does it have to be in external RAM?
    b64_buffer = (char*)heap_caps_malloc(b64_size, MALLOC_CAP_SPIRAM);
    if ( b64_buffer == NULL) {
        ESP_LOGE(__func__, "Failed to allocate memory in PSRAM");
        return;
    }

    // initialize components ...

    // gpio
    gpio_set_direction(GPIO_NUM_33, GPIO_MODE_OUTPUT);

    // nvs
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // camera
    ESP_ERROR_CHECK(cam.init_camera());

    // WiFi
    wifi.init();
    wifi.register_event_callback( IP_EVENT, IP_EVENT_STA_GOT_IP, [](auto event_data) {
        auto event = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(__func__, "wifi connected with IP: " IPSTR, IP2STR(&event->ip_info.ip));
        start_mqtt_client();
    });

    // queues
    camera_evt_queue = xQueueCreate(10, sizeof(uint8_t));

    // tasks
    xTaskCreate(camera_task, "camera", 4096, NULL, 5, NULL);

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
            ESP_LOGI(__func__, "image sent");

            cam.free_buffer();

            gpio_set_level(GPIO_NUM_33, 1);
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
        else{
            ESP_LOGE(__func__, "MQTT not connected");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

void start_mqtt_client(){
    //initialize mqtt client
    mqtt.init();

    mqtt.register_event_callback(MQTT_EVENT_CONNECTED, [](auto event_data) {
        ESP_LOGI(__func__, "MQTT_connected");
        mqtt.subscribe("/camera/cmd", 0);
    });

    // Triggered when the client receives data from a subscribed topic
    mqtt.register_event_callback(MQTT_EVENT_DATA, [](auto event_data) {
        uint8_t cmd = 1; //dummy data to send to the queue
        ESP_LOGI(__func__, "MQTT_EVENT_DATA");
        ESP_LOGI(__func__, "Received topic: %.*s", event_data->topic_len, event_data->topic);
        ESP_LOGI(__func__, "Received data: %.*s", event_data->data_len, event_data->data);

        // Check if the received message is a snap command
        if (strncmp(event_data->topic, "/camera/cmd", event_data->topic_len) == 0) {
            if (strncmp(event_data->data, "snap", event_data->data_len) == 0) {
                ESP_LOGI(__func__, "snap command received");
                xQueueSend(camera_evt_queue, &cmd, 0);
            }
        }
    });

    ESP_LOGI(__func__, "mqtt client initialized");
}

void base64_encode(const uint8_t *input, size_t input_len, char *output, size_t output_len) {
    size_t olen = 0;
    int ret = mbedtls_base64_encode((unsigned char *)output, output_len, &olen, input, input_len);
    if (ret != 0) {
        ESP_LOGE("BASE64", "Base64 encoding failed with error code: %d", ret);
    } }
