#pragma once
#include <atomic>
#include <functional>
#include <unordered_map>
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>

/**
 * @brief A thin wrapper around the ESP-IDF "esp_wifi" for quick use.
 *
 * This class provides a simplified interface for managing WiFi connections
 * using the ESP-IDF framework. It allows for event handling and connection
 * management.
 */
class WiFiStation {
public:
    /**
     * @brief Type definition for WiFi event callback functions.
     *
     * This is a function object type that can be registered for WiFi events.
     */
    using WifiEventCallback = std::function<void(void*)>;

    /**
     * @brief Constructor for the WiFiStation instances.
     *
     * Initializes the WiFiStation with the provided SSID and password.
     *
     * @param ssid The SSID of the WiFi network.
     * @param password The password for the WiFi network.
     */
    WiFiStation(const char* ssid, const char* password);

    /**
     * @brief Initialize the WiFiStation object.
     *
     * This method must be called before using the WiFiStation object
     * to set up the necessary configurations and event handlers.
     */
    void init();

    /**
     * @brief Register a callback for a specific WiFi event.
     *
     * This method allows the user to register a callback function
     * that will be invoked when the specified event occurs.
     *
     * @param event_base The base ID of the event to register the handler for.
     * @param event_id The identifier of the event to register the handler for.
     * @param callback The callback function to register for the given event.
     *
     */
    //TODO: This method is currently complex; consider simplifying it
    //       if only a connection event callback is needed.
    void register_event_callback(esp_event_base_t event_base, int32_t event_id, WifiEventCallback callback);

    /**
     * @brief Check if the WiFi is connected.
     *
     * This method checks the current connection status of the WiFi.
     *
     * @return true if the WiFi is connected, false otherwise.
     */
    bool is_connected() const;

private:
    static constexpr const char* TAG = "WIFI"; ///< Log tag for WiFi operations.
    const char* ssid_; ///< The SSID of the WiFi network.
    const char* password_; ///< The password for the WiFi network.
    std::atomic<bool> is_connected_; ///< Atomic flag indicating connection status.

    static void event_handler(void* , esp_event_base_t, int32_t, void*);
    void handle(esp_event_base_t event_base, int32_t event_id, void* event_data);

    // Set default handlers for Wi-Fi and IP events
    void set_default_handlers();

    /**
     * this class is used for registering the "event_callbacks_"
     */
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& pair) const {
            return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
        }
    };
    std::unordered_map<std::pair<esp_event_base_t, int32_t>, WifiEventCallback, pair_hash> event_callbacks_; ///< Map of registered event callbacks.
};
