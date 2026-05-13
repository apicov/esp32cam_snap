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

/* globals */
const char *TAG = "main";
MQTTClient *mqtt = nullptr;

// A FreeRTOS queue acts as a thread-safe FIFO between producers (MQTT callback,
// GPIO ISR) and the consumer (camera_task). Using a queue avoids race conditions
// and keeps ISR work minimal — the ISR just posts a token; the heavy lifting
// (capture + encode + publish) happens in the task context.
QueueHandle_t camera_evt_queue = nullptr;

/* prototypes */
void camera_task(void *p);
void start_mqtt_client();

// IRAM_ATTR places this function in Internal RAM instead of flash.
// ISRs must live in IRAM because flash may be temporarily unavailable
// (e.g. during flash reads/writes via SPI bus), and an ISR running from
// flash during that window would crash the system.
static void IRAM_ATTR gpio13_isr_handler(void *arg)
{
    uint8_t cmd = 1;

    // In an ISR we cannot use blocking FreeRTOS calls (no sleeping, no waiting
    // for mutexes). xQueueSendFromISR is the ISR-safe variant: it never blocks
    // and signals the scheduler if a higher-priority task was unblocked.
    BaseType_t high_task_woken = pdFALSE;
    xQueueSendFromISR(camera_evt_queue, &cmd, &high_task_woken);

    // If sending to the queue unblocked camera_task (which has higher priority
    // than whatever was interrupted), yield immediately so it runs right away
    // instead of waiting for the next scheduler tick.
    if (high_task_woken) portYIELD_FROM_ISR();
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "Starting application...");

    // gpio
    gpio_set_direction(GPIO_NUM_33, GPIO_MODE_OUTPUT);

    // NVS (Non-Volatile Storage) is a key-value store in flash. Many ESP-IDF
    // components (WiFi, BT) rely on it to persist state across reboots.
    // The erase-and-reinit path handles two error cases:
    //   - NVS_NO_FREE_PAGES: the partition is full (e.g. after an OTA update).
    //   - NVS_NEW_VERSION_FOUND: the stored format is incompatible with this firmware.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // WiFi startup is asynchronous: start() kicks off the connection and returns
    // immediately. on_connect registers a callback that runs once the station
    // is associated and has an IP, at which point it is safe to open the MQTT
    // connection (which requires a network).
    WiFiStation::start(CONF(WIFI_SSID), CONF(WIFI_PASSWORD)).on_connect(
        [](auto _){ start_mqtt_client(); }
    );

    // Queue depth of 10: if camera_task is busy, up to 10 trigger events
    // (from GPIO or MQTT) can be buffered before new ones are silently dropped.
    camera_evt_queue = xQueueCreate(10, sizeof(uint8_t));

    // Configure GPIO13 as a rising-edge interrupt input.
    // pin_bit_mask uses a 64-bit bitmask so multiple pins could be configured
    // with one call by OR-ing their bit positions.
    // PULLDOWN keeps the line at a known logic-0 when the sensor is inactive,
    // ensuring the transition to logic-1 is clean and unambiguous.
    // POSEDGE fires the ISR only on 0→1 transitions, ignoring the falling edge.
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_13),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io_conf);

    // gpio_install_isr_service allocates a shared ISR dispatcher that routes
    // GPIO interrupts to per-pin handlers registered with gpio_isr_handler_add.
    // It must be called once before any handler can be added.
    // The flag argument (0) selects the default ESP_INTR_FLAG_* options.
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_13, gpio13_isr_handler, nullptr);

    // camera_task is a FreeRTOS task: an independent thread with its own stack
    // (4096 bytes here). Priority 5 means it preempts lower-priority tasks
    // when the queue delivers an event.
    xTaskCreate(camera_task, "camera", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "esp32cam_snap is running");
} // end of app_main


void camera_task(void *p)
{
    CameraCtl cam{};
    uint8_t cmd;

    while(1)
    {
        // portMAX_DELAY blocks indefinitely until an item arrives in the queue.
        // This is more efficient than polling: the task is suspended by the
        // scheduler and consumes no CPU while waiting.
        xQueueReceive(camera_evt_queue, &cmd, portMAX_DELAY);

        if(mqtt && mqtt->is_connected()) {
            // capture_do acquires the camera frame buffer, calls the lambda,
            // then releases the buffer. The lambda must not store a pointer to
            // the buffer past its own scope.
            cam.capture_do([](const auto &pic){
                auto jpeg_data = pic.image();
                auto jpeg_len = pic.size();

                ESP_LOGI(TAG, "Captured JPEG image: %zu bytes", jpeg_len);

                // MQTT payloads are text-based, so the binary JPEG must be
                // Base64-encoded. Base64 expands every 3 bytes into 4 ASCII
                // characters, hence the ceiling formula: ceil(n/3)*4, plus one
                // byte for the null terminator.
                size_t jpeg_b64_size = (4 * ((jpeg_len + 2) / 3)) + 1;

                // MALLOC_CAP_SPIRAM allocates from external PSRAM (when present)
                // instead of the limited internal SRAM (~300 KB on ESP32).
                // Large buffers (image data, base64 output) must go to PSRAM.
                char* jpeg_b64_buffer = (char*)heap_caps_malloc(jpeg_b64_size, MALLOC_CAP_SPIRAM);

                if (!jpeg_b64_buffer) {
                    ESP_LOGE(TAG, "Failed to allocate base64 buffer for JPEG");
                    return;
                }

                size_t olen;
                auto ret = mbedtls_base64_encode(
                    (unsigned char *)jpeg_b64_buffer, jpeg_b64_size, &olen, jpeg_data, jpeg_len);

                if (ret == 0) {
                    ESP_LOGI(TAG, "Publishing %zu bytes (base64) to MQTT - original JPEG", olen);
                    mqtt->publish(CONF(IMAGE_TOPIC), jpeg_b64_buffer, 0, 0);
                    ESP_LOGI(TAG, "Original JPEG published successfully");
                }
                else {
                    ESP_LOGE(TAG, "Base64 encoding failed");
                }

                heap_caps_free(jpeg_b64_buffer);

                // The 160x120 JPEG is decoded to raw RGB888 for local processing
                // (e.g. running an ML model). Then it is downscaled to 96x96,
                // a common input resolution for tinyML image classifiers.
                // constexpr size_t decoded_size = 160 * 120 * 3;  // RGB888: 57,600 bytes
                // constexpr size_t resized_size = 96 * 96 * 3;    // RGB888: 27,648 bytes

                // uint8_t* decoded_buf = (uint8_t*)heap_caps_malloc(decoded_size, MALLOC_CAP_SPIRAM);
                // uint8_t* resized_buf = (uint8_t*)heap_caps_malloc(resized_size, MALLOC_CAP_SPIRAM);

                // if (!decoded_buf || !resized_buf) {
                //     ESP_LOGE(TAG, "Failed to allocate processing buffers in PSRAM");
                //     heap_caps_free(decoded_buf);
                //     heap_caps_free(resized_buf);
                //     return;
                // }

                // fmt2rgb888 decodes the JPEG into a flat RGB888 byte array
                // (R, G, B per pixel, row-major). PIXFORMAT_JPEG tells the
                // decoder what format the source data is in.
                // if (!fmt2rgb888(jpeg_data, jpeg_len, PIXFORMAT_JPEG, decoded_buf)) {
                //     ESP_LOGE(TAG, "JPEG to RGB888 conversion failed");
                //     heap_caps_free(decoded_buf);
                //     heap_caps_free(resized_buf);
                //     return;
                // }
                // ESP_LOGI(TAG, "JPEG decoded to RGB888");

                // resizeColorImage(decoded_buf, 160, 120, resized_buf, 96, 96);
                // ESP_LOGI(TAG, "Image resized to 96x96 for local processing");

                // heap_caps_free(decoded_buf);
                // heap_caps_free(resized_buf);
            });
        }
        else
            ESP_LOGE(TAG, "MQTT not connected");

        // Brief delay after a capture to allow the camera sensor to stabilize
        // before the next frame can be requested.
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void start_mqtt_client(){
    // reconnect if the client was created already
    if (mqtt) {
        mqtt->reconnect();
        return;
    }

    // on_connect and on_data_received register callbacks that run inside the
    // MQTT event loop (a separate FreeRTOS task managed by esp-mqtt).
    // Callbacks must be short and non-blocking; heavier work is deferred via
    // the queue to camera_task.
    mqtt = new MQTTClient{CONF(BROKER_URI)};
    mqtt->on_connect([](auto _) { mqtt->subscribe(CONF(CMD_TOPIC)); });
    mqtt->on_data_received([](auto data) {
        uint8_t cmd = 1; //dummy data to send to the queue
        ESP_LOGI(TAG, "Received on topic: %.*s", data->topic_len, data->topic);
        ESP_LOGI(TAG, "Received command: '%.*s'", data->data_len, data->data);

        // strncmp with topic_len avoids reading past the (non-null-terminated)
        // topic buffer that esp-mqtt provides.
        if (strncmp(data->topic, CONF(CMD_TOPIC), data->topic_len) == 0) {
            if (strncmp(data->data, "snap", data->data_len) == 0)
                xQueueSend(camera_evt_queue, &cmd, 0);  // 0 = don't wait if queue is full
        }
    });
}
