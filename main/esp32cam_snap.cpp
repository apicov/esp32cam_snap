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
#include <image_utils.h>
#include <esp_camera.h>

// shorten CONFIG names
#define CONF(name) CONFIG_CAM_MQTT_ ## name

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

    while(1)
    {
        //wait for mqtt command
        xQueueReceive(camera_evt_queue, &cmd, portMAX_DELAY);

        if(mqtt && mqtt->is_connected()) {
            cam.capture_do([](const auto &pic){
                auto jpeg_data = pic.image();
                auto jpeg_len = pic.size();

                ESP_LOGI(TAG, "Captured JPEG image: %zu bytes", jpeg_len);

                // Allocate buffers in PSRAM for image processing
                constexpr size_t decoded_size = 160 * 120 * 3;  // RGB888: 57,600 bytes
                constexpr size_t resized_size = 96 * 96 * 3;    // RGB888: 27,648 bytes
                constexpr size_t b64_size = (4 * ((resized_size + 2) / 3)) + 1;  // ~36,865 bytes

                uint8_t* decoded_buf = (uint8_t*)heap_caps_malloc(decoded_size, MALLOC_CAP_SPIRAM);
                uint8_t* resized_buf = (uint8_t*)heap_caps_malloc(resized_size, MALLOC_CAP_SPIRAM);
                char* b64_buffer = (char*)heap_caps_malloc(b64_size, MALLOC_CAP_SPIRAM);

                if (!decoded_buf || !resized_buf || !b64_buffer) {
                    ESP_LOGE(TAG, "Failed to allocate processing buffers in PSRAM");
                    heap_caps_free(decoded_buf);
                    heap_caps_free(resized_buf);
                    heap_caps_free(b64_buffer);
                    return;
                }

                ESP_LOGI(TAG, "Allocated buffers: decoded=%p, resized=%p, b64=%p (all in PSRAM)",
                         decoded_buf, resized_buf, b64_buffer);

                // Convert JPEG to RGB888 (160x120)
                if (!fmt2rgb888(jpeg_data, jpeg_len, PIXFORMAT_JPEG, decoded_buf)) {
                    ESP_LOGE(TAG, "JPEG to RGB888 conversion failed");
                    heap_caps_free(decoded_buf);
                    heap_caps_free(resized_buf);
                    heap_caps_free(b64_buffer);
                    return;
                }
                ESP_LOGI(TAG, "JPEG decoded to RGB888");

                // Resize from 160x120 to 96x96
                resizeColorImage(decoded_buf, 160, 120, resized_buf, 96, 96);
                ESP_LOGI(TAG, "Image resized to 96x96");

                // Base64 encode the resized image
                size_t olen;
                auto ret = mbedtls_base64_encode(
                    (unsigned char *)b64_buffer, b64_size, &olen, resized_buf, resized_size);

                if (ret == 0) {
                    ESP_LOGI(TAG, "Publishing %zu bytes (base64) to MQTT", olen);
                    mqtt->publish(CONF(IMAGE_TOPIC), b64_buffer, 2, 0);
                    ESP_LOGI(TAG, "Image published successfully");
                }
                else {
                    ESP_LOGE(TAG, "Base64 encoding failed, buffer too small (%zu), needs %zu",
                             b64_size, olen);
                }

                // Free buffers
                heap_caps_free(decoded_buf);
                heap_caps_free(resized_buf);
                heap_caps_free(b64_buffer);
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

    mqtt = new MQTTClient{CONF(BROKER_URI)};
    mqtt->on_connect([](auto _) { mqtt->subscribe(CONF(CMD_TOPIC)); });
    mqtt->on_data_received([](auto data) {
        uint8_t cmd = 1; //dummy data to send to the queue
        ESP_LOGI(TAG, "Received on topic: %.*s", data->topic_len, data->topic);
        ESP_LOGI(TAG, "Received command: '%.*s'", data->data_len, data->data);

        // Check if the received message is a snap command
        if (strncmp(data->topic, CONF(CMD_TOPIC), data->topic_len) == 0) {
            if (strncmp(data->data, "snap", data->data_len) == 0)
                xQueueSend(camera_evt_queue, &cmd, 0);
        }
    });
}
