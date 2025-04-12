#pragma once
#include <atomic>
#include <cstring>
#include <functional>
#include <vector>

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"


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
     * @brief Startup the WiFi in station mode.
     *
     * If called more than once, it'll initialize the station only
     * once and will return the same instance after each call.
     *
     * @param ssid The SSID of the WiFi network.
     * @param password The password for the WiFi network.
     *
     * @return The instance of the WiFiStation.
     */
    static WiFiStation& start(const char*, const char*);

    /**
     * @brief Register a callback on a connection event.
     *
     * @param The callback to execute.
     *
     */
    void on_connect(WifiEventCallback);

    /**
     * @brief Check if the WiFi is connected.
     *
     * This method checks the current connection status of the WiFi.
     *
     * @return true if the WiFi is connected, false otherwise.
     */
    bool is_connected() const;

private:
    static constexpr const char* TAG = "wifi_station";
    static WiFiStation* instance;
    const char* ssid_; ///< The SSID of the WiFi network.
    const char* password_; ///< The password for the WiFi network.
    std::atomic<bool> is_connected_; ///< Atomic flag indicating connection status.
    std::vector<WifiEventCallback> on_connect_cb;

    WiFiStation(const char* ssid, const char* password);
    WiFiStation() = delete;
    WiFiStation& operator=(WiFiStation const&) = delete;

    static void event_handler(void* , esp_event_base_t, int32_t, void*);
    void handle(esp_event_base_t , int32_t, void*);
};
