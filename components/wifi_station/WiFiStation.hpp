#pragma once
#include <atomic>
#include <functional>
#include <unordered_map>
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>

// Hash function for std::pair
// TODO: this should be private enclosed within WiFiStation
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

/**
 * @brief thin wrapper around "esp_wifi" for quick use.
 *
 */
class WiFiStation {
public:
    /**
     * @brief "FunctionObject" type that can be registered for WIFI events.
     *
     */
    using WifiEventCallback = std::function<void(void*)>;

    /**
     * @brief Constructor of the WiFiStation istances.
     *
     * @param ssid string.
     * @param password string.
     *
     */
    WiFiStation(const char* ssid, const char* password);

    /**
     * @brief Initialize the WiFiStation object.
     *
     * @note This needs to be called before using the object.
     *
     */
    void init();

    /**
     * @brief Register a callback to a given event.
     *
     * @param event_base is the base ID of the event to register the handler for.
     * @param event_id is the identifier of the event to register the handler for.
     * @param callback to register on the given event
     */
    // TODO: this method is way too commplicated; if we only need it to register the a
    // callback to the "connect" event, then probably it should be simplified in a
    // "on_connect" method that only takes the "callback" argument
    void register_event_callback(esp_event_base_t event_base, int32_t event_id, WifiEventCallback callback);

    /**
     * @brief Test whether the WiFi is connected
     *
     * @return "true" if the WiFis is connected or "false" otherwise.
     */
    bool is_connected() const;

private:
    const char* ssid_;
    const char* password_;
    const char* mqtt_broker_uri_; // XXX: does this variable belong to this module?
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
