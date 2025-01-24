#include <esp_system.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "driver/gpio.h"
//#include <inttypes.h> // For PRIu32

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_log.h"
//#include "esp_wifi.h"
//#include "esp_event.h"
//#include "esp_netif.h"
//#include "esp_http_client.h"


//#include "mqtt_client.h"
//#include "private_data.h"
#include "camera_ctl.h"
#include "sd_card.h"



#include <stdint.h>//lib for ints i-e int8_t upto 1int64_t
#include <string.h> //for string data i-e char array
#include <stdbool.h> //for boolean data type
#include <stdio.h> // macros,input output, files etc
#include "nvs.h" //non volatile storage important for saving data while code runs
#include "nvs_flash.h" // storing in nvs flash memory
#include "freertos/FreeRTOS.h" //freertos for realtime opertaitons
#include "freertos/task.h" // creating a task handler and assigning priority
#include "esp_log.h" // printing out logs info this is to avoid printf statment

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "esp_gap_bt_api.h"


#define SWITCH_GPIO GPIO_NUM_16
static const char *TAG = "WIFI_STA"; // Tag for logging


QueueHandle_t gpio_evt_queue = NULL;  // FreeRTOS queue for GPIO events
QueueHandle_t camera_evt_queue = NULL;  // FreeRTOS queue for camera trigger events


//static atomic_bool is_wifi_connected = false; // Atomic flag for Wi-Fi connection status
//static esp_mqtt_client_handle_t mqtt_client = NULL; // Global MQTT client handle


void camera_task(void *p);
void gpio_task(void *p);


#define DEVICE_NAME "ESP32_BT_SPP"

// Bluetooth SPP configuration
esp_spp_cfg_t spp_cfg = {
    .mode = ESP_SPP_MODE_CB, // Use callback mode
    .enable_l2cap_ertm = false,
    .tx_buffer_size = 0, // Default buffer size
};

// Callback for SPP events
void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(TAG, "SPP initialized, starting discovery...");
        esp_bt_gap_set_device_name(DEVICE_NAME); // Set device name
        esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, DEVICE_NAME);
        break;

    case ESP_SPP_START_EVT:
        ESP_LOGI(TAG, "SPP server started, waiting for connections...");
        break;

    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(TAG, "SPP connection opened (handle: %" PRIu32 ")", param->srv_open.handle);
        break;

    case ESP_SPP_DATA_IND_EVT:
        ESP_LOGI(TAG, "SPP data received (len: %d): %.*s",
                 param->data_ind.len, param->data_ind.len, param->data_ind.data);
        // Echo back received data
        //esp_spp_write(param->data_ind.handle, param->data_ind.len, param->data_ind.data);
        if (strncmp( (char * )param->data_ind.data, "p",1) == 0)
            {
                uint8_t i = 1;
                xQueueSend(camera_evt_queue, &i, portMAX_DELAY);
                ESP_LOGI(TAG, "llego p"); 
            }
        break;

    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(TAG, "SPP data sent (len: %d)", param->write.len);
        break;

    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(TAG, "SPP connection closed (handle: %" PRIu32 ")", param->close.handle);
        break;

    default:
        ESP_LOGI(TAG, "Unhandled SPP event: %d", event);
        break;
    }
}

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

    // Initialize the Bluetooth controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);  
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize the Bluetooth stack 
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return;
    }

    // Register the SPP callback
    esp_spp_register_callback(esp_spp_cb);

    // Initialize the SPP profile
    ret = esp_spp_enhanced_init(&spp_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPP initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // start the spp server
    esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, DEVICE_NAME);

    esp_bt_gap_set_device_name(DEVICE_NAME);
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    
    ret = initi_sd_card();
    if (ret != ESP_OK) {
        ESP_LOGE("SD_CARD", "initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    xTaskCreate(camera_task, "camera",8192, NULL, 5, NULL);
    //xTaskCreate(gpio_task, "main", 4096, NULL, 5, NULL);

    //gpio_install_isr_service(0);
    //gpio_isr_handler_add(SWITCH_GPIO, gpio_isr_handler, (void *)SWITCH_GPIO);
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
        cam.capture_to_file(photo_name);
        gpio_set_level(GPIO_NUM_33, 1);
    }
}
