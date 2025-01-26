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
#include "camera_ctl.h"
#include "sd_card.h"

#include "esp_wifi.h"
#include <stdatomic.h> 
static atomic_bool is_wifi_connected = false; // Atomic flag for Wi-Fi connection status
void wifi_init_sta();  // Initialize Wi-Fi in Station (STA) mode
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);  // Wi-Fi event handler


#include <stdint.h>//lib for ints i-e int8_t upto 1int64_t
#include <string.h> //for string data i-e char array
#include <stdbool.h> //for boolean data type
#include <stdio.h> // macros,input output, files etc
#include "nvs.h" //non volatile storage important for saving data while code runs
#include "nvs_flash.h" // storing in nvs flash memory
#include "freertos/FreeRTOS.h" //freertos for realtime opertaitons
#include "freertos/task.h" // creating a task handler and assigning priority

#define SWITCH_GPIO GPIO_NUM_16
static const char *TAG = "WIFI_STA"; // Tag for logging


QueueHandle_t gpio_evt_queue = NULL;  // FreeRTOS queue for GPIO events
QueueHandle_t camera_evt_queue = NULL;  // FreeRTOS queue for camera trigger events

//This macro explicitly places the variable in external PSRAM.
uint8_t  *img_buffer; 
void save_cam_image(char *fname, camera_fb_t *pic, uint8_t *img_buffer);



//static esp_mqtt_client_handle_t mqtt_client = NULL; // Global MQTT client handle


void camera_task(void *p);
void gpio_task(void *p);



/*
void IRAM_ATTR gpio_isr_handler(void* arg)
{
    int gpio_num = (int)arg; // Get GPIO number
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL); // Send to queue
}
*/

extern "C" void app_main()
{
    ESP_LOGI(TAG, "application started");

    // Allocate for color image (RGB) in external ram
   img_buffer = (uint8_t*)heap_caps_malloc(160 * 120 * 3, MALLOC_CAP_SPIRAM);
    if (img_buffer == NULL) {
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

    wifi_init_sta(); // Initialize Wi-Fi in Station (STA) mode

    sdmmc_card_t *card;
    ret = initi_sd_card("/sdcard", &card);
    if (ret != ESP_OK) {
        ESP_LOGE("SD_CARD", "initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    xTaskCreate(camera_task, "camera",4096, NULL, 5, NULL);
    //xTaskCreate(gpio_task, "main", 4096, NULL, 5, NULL);

    //gpio_install_isr_service(0);
    //gpio_isr_handler_add(SWITCH_GPIO, gpio_isr_handler, (void *)SWITCH_GPIO);

    //heap_caps_free(img_buffer); // Free when done
} // end of app_main   

void gpio_task(void* arg)
{
    int gpio_num;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY))
        {
            printf("GPIO %d interrupt received\n", gpio_num);
            // Perform further processing (e.g., debounce logic)
        }
    }
}

CameraCtl cam;

void camera_task(void *p)
{
    
    char photo_name[50];

    esp_err_t err;
    err = cam.init_camera();
    if (err != ESP_OK)
    {
        printf("err: %s\n", esp_err_to_name(err));
        return;
    }
    ESP_LOGE(TAG, "camera initialized");
    unsigned int i = 0;
    uint8_t cmd;

    while(1)
    {
        xQueueReceive(camera_evt_queue, &cmd, portMAX_DELAY);

        gpio_set_level(GPIO_NUM_33, 0);
        //vTaskDelay(2500 / portTICK_PERIOD_MS);
        //gpio_set_level(GPIO_NUM_33, 0);
        //vTaskDelay(2500 / portTICK_PERIOD_MS);
        
        sprintf(photo_name, "/sdcard/pic_%u.ppm", i++);
        cam.capture();
        save_cam_image(photo_name, cam.pic, img_buffer);
        cam.free_buffer();
        ESP_LOGI(CAM_TAG, "Finished Taking Picture!");

        gpio_set_level(GPIO_NUM_33, 1);
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



void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI("WIFI_STA", "Wi-Fi disconnected, retrying...");
        atomic_store(&is_wifi_connected, false); // Update flag atomically
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI("WIFI_STA", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        atomic_store(&is_wifi_connected, true); // Update flag atomically
    }
}


// Initialize Wi-Fi in Station (STA) mode
void wifi_init_sta() {
    // Initialize the TCP/IP stack
    esp_netif_init();

    // Create the default event loop
    esp_event_loop_create_default();

    // Create a default network interface for Wi-Fi station
    esp_netif_create_default_wifi_sta();

    // Initialize Wi-Fi driver with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Register event handlers for Wi-Fi and IP events
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

    // Configure Wi-Fi connection settings
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PASSWORD,
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
        },
    };

    // Set Wi-Fi mode to Station
    esp_wifi_set_mode(WIFI_MODE_STA);

    // Apply the Wi-Fi configuration
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    // Start the Wi-Fi driver
    esp_wifi_start();

    ESP_LOGI("WIFI_STA", "Wi-Fi initialized in Station mode");
}