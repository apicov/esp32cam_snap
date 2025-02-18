#pragma once
#include <atomic>
#include <functional>
#include <unordered_map>
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>

// Hash function for std::pair
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

class WiFiStation {
public:
    // Type alias for event callback
    using WifiEventCallback = std::function<void(void*)>;
    WiFiStation(const char* ssid, const char* password);
    void init();
    void register_event_callback(esp_event_base_t event_base, int32_t event_id, WifiEventCallback callback);
    bool is_connected() const;

private:
    const char* ssid_;
    const char* password_;
    const char* mqtt_broker_uri_;
    std::atomic<bool> is_connected_;
    esp_event_handler_instance_t instance_any_id_;
    esp_event_handler_instance_t instance_ip_event_;
    std::unordered_map<std::pair<esp_event_base_t, int32_t>, WifiEventCallback, pair_hash> event_callbacks_;

    static constexpr const char* TAG = "WIFI";

    // Static event handler required by the ESP-IDF
    static void event_handler_static(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    // Instance-level event handler
    void event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data);

    // Set default handlers for Wi-Fi and IP events
    void set_default_handlers();

    
};
