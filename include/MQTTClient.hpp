#pragma once
#include <atomic>
#include <functional>

#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_log.h"


class MQTTClient {
  public:  
    using MQTTEventCallback = std::function<void(esp_mqtt_event_handle_t)>;
    MQTTClient(const char* uri);
    void init();
    void register_event_callback(int32_t event_id, MQTTEventCallback callback);
    bool is_connected() const;
    void publish(const char* topic, const char* data, int qos, int retain); 
    esp_err_t subscribe(const char* topic, int qos);
  private:
    std::atomic<bool> is_connected_;
    esp_mqtt_client_handle_t mqtt_client_ = NULL; // Global MQTT client handle
    std::unordered_map<int32_t, MQTTEventCallback> event_callbacks_;
    const char* mqtt_broker_uri_;
    static constexpr const char* TAG = "MQTTCLIENT";
    // Static event handler required by the ESP-IDF
    static void event_handler_static(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    // Instance-level event handler
    void event_handler(esp_mqtt_event_handle_t event_data); 
    // Set default handlers for Wi-Fi and IP events
    void set_default_handlers();
  };
