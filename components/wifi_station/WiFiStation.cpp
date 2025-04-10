#include "WiFiStation.hpp"

WiFiStation::WiFiStation(const char* ssid, const char* password)
    : ssid_(ssid), password_(password), is_connected_(false) {}

void WiFiStation::init() {
    // Initialize the TCP/IP stack
    esp_netif_init();

    // Create the default event loop
    esp_event_loop_create_default();

    // Create a default network interface for Wi-Fi station
    esp_netif_create_default_wifi_sta();

    // Initialize Wi-Fi driver with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Register the static event handler
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, this, NULL);

    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, event_handler, this, NULL);

    // Configure Wi-Fi connection settings
    wifi_config_t wifi_config = {};
    strcpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid_);
    strcpy(reinterpret_cast<char*>(wifi_config.sta.password), password_);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    // Set Wi-Fi mode to Station
    esp_wifi_set_mode(WIFI_MODE_STA);

    // Apply the Wi-Fi configuration
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    // Start the Wi-Fi driver
    esp_wifi_start();

    ESP_LOGI(TAG, "Wi-Fi initialized in Station mode");

}

void WiFiStation::register_event_callback(esp_event_base_t event_base, int32_t event_id, WifiEventCallback callback) {
    event_callbacks_[{event_base, event_id}] = std::move(callback);
}

bool WiFiStation::is_connected() const {
    return is_connected_.load();
}

// Static event handler required by the ESP-IDF
void WiFiStation::event_handler(void* this_, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    static_cast<WiFiStation*>(this_)->handle(event_base, event_id, event_data);
}

// Instance-level event handler
void WiFiStation::handle(esp_event_base_t base, int32_t id, void* data) {

    bool handled{true};

    // default handlers
    if (base == WIFI_EVENT) {
        switch(id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "started, connecting...");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            is_connected_.store(false);
            ESP_LOGI(TAG, "disconnected, retrying to connect...");
            esp_wifi_connect();
            break;
        }
    } else if(base == IP_EVENT) {
        switch(id) {
        case IP_EVENT_STA_GOT_IP:
            is_connected_.store(true);
            auto* event = static_cast<ip_event_got_ip_t*>(data);
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            break;
        }
    } else handled = false;

    // attend user handlers
    auto it = event_callbacks_.find(std::make_pair(base, id));
    if (it != event_callbacks_.end()) {
        // Call the registered callback for this event
        it->second(data);
    } else if (!handled) {
        ESP_LOGW(TAG, "Unhandled event: base=%s, id=%d", base, id);
    }
}
