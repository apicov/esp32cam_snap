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

// Local components
#include <camera_ctl.hpp>
#include <MQTTClient.hpp>
#include <WiFiStation.hpp>

// shorten CONFIG names
#define CONF(name) CONFIG_SNAP_ ## name

/* prototypes */
void camera_task(void *p);
void start_mqtt_client();

/* globals */
const char *TAG = "main";
MQTTClient *mqtt = nullptr;
QueueHandle_t camera_evt_queue = nullptr;  // FreeRTOS queue for camera trigger events

extern "C" void app_main()
{
    ESP_LOGI(TAG, "Starting application...");

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
    WiFiStation::start(CONF(WIFI_SSID), CONF(WIFI_PASSWORD)).on_connect(
      [](auto _){ start_mqtt_client(); }
    );

    // queues
    camera_evt_queue = xQueueCreate(10, sizeof(uint8_t));

    // tasks
    xTaskCreate(camera_task, "camera", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "esp32cam_snap is running");
} // end of app_main


void camera_task(void *p)
{
    CameraCtl cam{};
    uint8_t cmd;

    // TODO: if the image resolution can be adjusted, then it might be
    // a good idea to make the dimensions configurable
    constexpr size_t image_size{160 * 120 * 3};
    constexpr size_t b64_size{(4 * ((image_size + 2) / 3)) + 1};

    char* b64_buffer = (char*) malloc(b64_size);
    if (b64_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for encoding the image");
        // TODO: die here...
        return;
    }

    while(1)
    {
        //wait for mqtt command
        xQueueReceive(camera_evt_queue, &cmd, portMAX_DELAY);

        if(mqtt && mqtt->is_connected()) {
            cam.capture_do([b64_buffer](const auto &pic){
                auto src = pic.image();
                auto slen = pic.size();
                size_t olen;

                auto ret = mbedtls_base64_encode(
                  (unsigned char *) b64_buffer, b64_size, &olen, src, slen);

                if (ret == 0) {
                  mqtt->publish(CONF(MQTT_IMG_TOPIC), b64_buffer, 2, 0);
                }
                else {
                  ESP_LOGE(TAG, "the dest buffer is too small (%zu)"
                           ", it requires a length of %zu", b64_size, olen);
                }
            });
        }
        else
            ESP_LOGE(TAG, "MQTT not connected");

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void start_mqtt_client(){
    // reconnect if the client was created already
    if (mqtt) {
        mqtt->reconnect();
        return;
    }

    mqtt = new MQTTClient{CONF(MQTT_URI)};
    mqtt->on_connect([](auto _) { mqtt->subscribe(CONF(MQTT_CMD_TOPIC)); });
    mqtt->on_data_received([](auto data) {
        uint8_t cmd = 1; //dummy data to send to the queue
        ESP_LOGI(TAG, "Received on topic: %.*s", data->topic_len, data->topic);
        ESP_LOGI(TAG, "Received command: '%.*s'", data->data_len, data->data);

        // Check if the received message is a snap command
        if (strncmp(data->topic, CONF(MQTT_CMD_TOPIC), data->topic_len) == 0) {
            if (strncmp(data->data, "snap", data->data_len) == 0)
                xQueueSend(camera_evt_queue, &cmd, 0);
        }
    });
}
