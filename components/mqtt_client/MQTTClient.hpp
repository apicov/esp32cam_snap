#pragma once
#include <atomic>
#include <functional>
#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_log.h"

/**
 * @brief Thin wrapper around "mqtt_client" to create a quick MQTT client.
 *
 * This class provides a simplified interface for managing MQTT connections
 * and interactions using the ESP-IDF framework. It allows for event handling,
 * publishing messages, and subscribing to topics.
 */
class MQTTClient {
public:
    /**
     * @brief A "FunctionObject" type whose instances serve as callbacks for MQTT events.
     *
     * This type is used to define callback functions that will be invoked
     * when specific MQTT events occur.
     */
    using MQTTEventCallback = std::function<void(esp_mqtt_event_handle_t)>;

    /**
     * @brief Client constructor.
     *
     * Initializes the MQTTClient with the URI of the broker to connect to.
     *
     * @param uri The URI of the broker to connect to, using the format "mqtt://host:port".
     */
    MQTTClient(const char* uri);

    /**
     * @brief Register a callback for a given event.
     *
     * This method allows the user to register a callback function
     * that will be invoked when the specified MQTT event occurs.
     *
     * @param event_id The event identifier, which can be one of:
     *    - MQTT_EVENT_CONNECTED for a successful connection event
     *    - MQTT_EVENT_DATA when new data is available
     *
     * @param callback The "FunctionObject" to execute when the event occurs.
     *
     */
    // TODO: It might be a better idea to encapsulate the "event_id" in an enum
    // so we don't pass the event details from "mqtt_client" to the user
    void register_event_callback(int32_t event_id, MQTTEventCallback callback);

    /**
     * @brief Predicate to check if the client is connected.
     *
     * This method checks the current connection status of the MQTT client.
     *
     * @return true if the client is connected, false otherwise.
     */
    bool is_connected() const;

    /**
     * @brief Send a publish message to the broker.
     *
     * This method publishes a message to the specified topic on the MQTT broker.
     *
     * @param topic The topic to which the message will be published.
     * @param data The payload of the message.
     * @param qos The Quality of Service level for the message.
     * @param retain The retain flag indicating whether the broker should retain the message.
     *
     */
    // TODO: For this application, are qos and retain relevant? maybe
    // default values can be assigned. Also if retain is a boolean then
    // it should be explained in the type
    void publish(const char* topic, const char* data, int qos, int retain);

    /**
     * @brief Subscribe the client to a topic.
     *
     * This method allows the client to subscribe to a specified topic
     * on the MQTT broker.
     *
     * @param topic The topic to subscribe to.
     * @param qos The Quality of Service level for the subscription.
     *
     * @note If QoS is not to be used much in this application, consider providing
     *       a sensible default value.
     * @return esp_err_t Error code indicating the result of the subscription attempt.
     */
    // TODO: if qos is not to be used much in this application, then maybe provide a
    // sane default value
    esp_err_t subscribe(const char* topic, int qos);

private:
    std::atomic<bool> is_connected_; ///< Atomic flag indicating connection status.
    esp_mqtt_client_handle_t mqtt_client_ = nullptr; ///< Global MQTT client handle.
    std::unordered_map<int32_t, MQTTEventCallback> event_callbacks_; ///< Map of registered event callbacks.
    const char* mqtt_broker_uri_; ///< URI of the MQTT broker.
    static constexpr const char* TAG = "MQTTCLIENT"; ///< Log tag for MQTT operations.

    /**
     * @brief Static event handler required by the ESP-IDF.
     *
     * This static method serves as the event handler for MQTT events.
     * It forwards the event to the appropriate instance-level handler.
     *
     * @param arg User-defined argument passed to the handler.
     * @param event_base The base ID of the event.
     * @param event_id The identifier of the event.
     * @param event_data Pointer to the event data.
     */
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    /**
     * @brief Instance-level event handler.
     *
     * This method handles MQTT events at the instance level, processing
     * the event data and invoking the registered callback if applicable.
     *
     * @param event_data The event data associated with the MQTT event.
     */
    void handle(esp_event_base_t, int32_t, esp_mqtt_event_handle_t);
};
