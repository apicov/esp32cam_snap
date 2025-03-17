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
#include <camera_ctl.hpp>
#include <MQTTClient.hpp>
#include <WiFiStation.hpp>

// Application configuration
#include "private_data.h"

/* prototypes */
void camera_task(void *p);
void mqtt_task(void *p);
void start_mqtt_client();
void maybe_send_image();

/* globals */
WiFiStation wifi(SSID, PASSWORD);
MQTTClient mqtt(MQTT_URI); //MQTT object with broker uri

QueueHandle_t camera_evt_queue = NULL;  // FreeRTOS queue for camera trigger events

// TODO: if the image resolution can be adjusted, then it might be
// a good idea to make the dimensions configurable
constexpr size_t image_size{160 * 120 * 3};

 //buffer for base64 encoding
constexpr size_t b64_size{(4 * ((image_size + 2) / 3)) + 1};
char  *b64_buffer;

extern "C" void app_main()
{
    ESP_LOGI(__func__, "starting...");

    // Allocate base64_encode in external ram
    // TODO: does it have to be in external RAM?
    b64_buffer = (char*) heap_caps_malloc(b64_size, MALLOC_CAP_SPIRAM);
    if (b64_buffer == NULL) {
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

    // WiFi
    wifi.init();
    wifi.register_event_callback(IP_EVENT, IP_EVENT_STA_GOT_IP, [](auto _) {
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
    static CameraCtl cam{};
    uint8_t cmd;

    while(1)
    {
        //wait for mqtt command
        xQueueReceive(camera_evt_queue, &cmd, portMAX_DELAY);

        if(mqtt.is_connected()){
            cam.capture_do([](const auto pic){
                auto src = pic->buf;
                auto slen = pic->len;
                size_t olen;

                auto ret = mbedtls_base64_encode(
                  (unsigned char *) b64_buffer, b64_size, &olen, src, slen);

                if (ret == 0) {
                  mqtt.publish("/camera/img", b64_buffer, 2, 0);
                  ESP_LOGI(__func__, "image sent");
                }
                else {
                  ESP_LOGE(__func__, "the dest buffer is too small (%zu)"
                           ", it requires a length of %zu", b64_size, olen);
                }
            });
        }
        else
            ESP_LOGE(__func__, "MQTT not connected");

        vTaskDelay(1000 / portTICK_PERIOD_MS);
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
