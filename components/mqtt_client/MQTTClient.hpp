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
     * @return esp_err_t Error code indicating the result of the subscription attempt.
     */
    // TODO: if qos is not to be used much in this application, then maybe provide a
    // sane default value
    esp_err_t subscribe(const char* topic, int qos=0);


    /**
     * @brief Force reconnection of the client
     *
     */
    esp_err_t reconnect(void);


    /**
     * @brief Enqueue an action to execute on a connect event
     *
     * @param MQTTEventCallback
     *
     * @return a reference to the same instance
     */
    MQTTClient& on_connect(MQTTEventCallback);

    /**
     * @brief Enqueue an action to execute on a disconnect event
     *
     * @param MQTTEventCallback
     *
     * @return a reference to the same instance
     */
    MQTTClient& on_disconnect(MQTTEventCallback);

    /**
     * @brief Enqueue an action to execute when data is received
     *
     * @param MQTTEventCallback
     *
     * @return a reference to the same instance
     */
    MQTTClient& on_data_received(MQTTEventCallback);

private:
    static constexpr const char* TAG = "mqtt_client"; ///< Log tag for MQTT operations.

    std::atomic<bool> is_connected_; ///< Atomic flag indicating connection status.
    esp_mqtt_client_handle_t mqtt_client_ = nullptr; ///< Global MQTT client handle.
    std::vector<MQTTEventCallback> on_connect_cb;
    std::vector<MQTTEventCallback> on_disconnect_cb;
    std::vector<MQTTEventCallback> on_data_received_cb;
    const char* mqtt_broker_uri_; ///< URI of the MQTT broker.

    static void event_handler(void*, esp_event_base_t, int32_t, void*);
    void handle(esp_event_base_t, int32_t, esp_mqtt_event_handle_t);
};
