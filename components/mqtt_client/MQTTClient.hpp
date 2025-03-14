#pragma once
#include <atomic>
#include <functional>

#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_log.h"

/**
 * @brief Thin wrapper around "mqtt_client" to create a quick MQTT client
 *
 */

class MQTTClient {
public:
    /**
     * @brief A "FunctionObject" type whose instances serve as callbacks for MQTT events.
     *
     */
    using MQTTEventCallback = std::function<void(esp_mqtt_event_handle_t)>;

    /**
     * @brief Client constructor
     *
     * @param uri of the broker to connect to, using the format "mqtt://host:port".
     */
    MQTTClient(const char* uri);

    /**
     * @brief Initializes the client instance.
     *
     * @note must be called before using the object.
     */
    // TODO: it might be a better idea to call this inside the default constructor
    // and not relying on the user to call it
    void init();

    /**
     * @brief Register a callback for a given event.
     *
     * @param event_id is the event identifier and can be one of:
     *    - MQTT_EVENT_CONNECTED for a successful connection event
     *    - MQTT_EVENT_DATA when new data is available
     *
     * @param callback is the "FunctionObject" to execute when an event occurs
     */
    // TODO: It might be a better idea to encapsulate the "event_id" in an enum
    // so we don't pass the event details from "mqtt_client" to the user
    void register_event_callback(int32_t event_id, MQTTEventCallback callback);

    /**
     * @brief Predicate to check if the client is connected
     *
     * @return "true" if the client is connected or "false" otherwise.
     */
    bool is_connected() const;

    /**
     * @brief Send a publish message to the broker
     *
     * @param topic of the message
     * @param data payload of the message
     * @param qos is the Quality of Service
     * @param retain flag
     *
     */
    // TODO: For this application, are qos and retain relevant? maybe
    // default values can be assigned. Also if retain is a boolean then
    // it should be explained in the type
    void publish(const char* topic, const char* data, int qos, int retain);

    /**
     * @brief Subscribe client to a topic
     *
     * @param topic string
     * @param qos is the Quality of Service
     *
     */
    // TODO: if qos is not to be used much in this application, then maybe provide a
    // sane default value
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
