#include "MQTTClient.hpp"

MQTTClient::MQTTClient(const char* mqtt_broker_uri)
    :mqtt_broker_uri_(mqtt_broker_uri) {

    ESP_LOGI(TAG, "Initializing MQTT client with URI: %s", mqtt_broker_uri_);

    // MQTT configuration
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = mqtt_broker_uri_;  // Correct URI assignment
    mqtt_cfg.session.keepalive = 10;  // Set the keep-alive interval

    // Initialize MQTT client
    mqtt_client_ = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client_) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    // Register the event handler
    esp_mqtt_client_register_event(mqtt_client_, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), event_handler, this);

    // Start the MQTT client
    esp_mqtt_client_start(mqtt_client_);

    ESP_LOGI("TAG", "MQTT client initialized and started");
}

// Static event handler required by the ESP-IDF
void MQTTClient::event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    auto* instance = static_cast<MQTTClient*>(arg);

    ESP_LOGD(TAG, "Received event id=%d", id);
    instance->handle(base, id, (esp_mqtt_event_handle_t)data);
}

// Instance-level event handler
void MQTTClient::handle(esp_event_base_t base, int32_t id, esp_mqtt_event_handle_t data) {

    //check if event is connected or disconnected to update is_connected_ variable
    if(id == MQTT_EVENT_CONNECTED) {
        is_connected_.store(true);
        ESP_LOGI(TAG, "The client is connected");
        // publish a test message to a "test" topic
        //
        // XXX: Probably this should be an optional feature, and it can
        // be guraded by a "define" macro so the user can enable it at
        // compile time
        int msg_id = esp_mqtt_client_publish(
            mqtt_client_, "/topic/test", "Hello from ESP32!", 0, 1, 0);
        if (msg_id < 0)
            ESP_LOGE(TAG, "Failed to publish in '/topic/test', msg_id=%d", msg_id);
    }
    else if (id == MQTT_EVENT_DISCONNECTED) {
        is_connected_.store(false);
    }
    else if (id == MQTT_EVENT_DATA) {
        ESP_LOGI(TAG, "Data received");
        ESP_LOGI(TAG, "Received topic: %.*s", data->topic_len, data->topic);
        ESP_LOGI(TAG, "Received data: %.*s", data->data_len, data->data);
    }

    auto key = data->event_id;
    auto it = event_callbacks_.find(key);

    if (it != event_callbacks_.end()) {
        // Call the registered callback for this event
        it->second(data);
    } else {
        ESP_LOGW(TAG, "Unhandled event: id=%d", data->event_id );
    }
}

void MQTTClient::register_event_callback(int32_t event_id, MQTTEventCallback callback) {
    event_callbacks_[event_id] = std::move(callback);
}

bool MQTTClient::is_connected() const {
    return is_connected_.load();
}

void MQTTClient::publish(const char* topic, const char* data, int qos, int retain) {
    // Check if the MQTT client is connected before attempting to publish
    if (!is_connected()) {
        ESP_LOGW(TAG, "MQTT client is not connected. Cannot publish message.");
        return;
    }

    // Publish the message to the specified topic with the given QoS and retain flag
    int msg_id = esp_mqtt_client_publish(mqtt_client_, topic, data, 0, qos, retain);

    // Check if the message was published successfully
    if (msg_id != -1) {
        ESP_LOGI(TAG, "Message published successfully, msg_id=%d", msg_id);
    } else {
        ESP_LOGE(TAG, "Failed to publish message");
    }
}

esp_err_t MQTTClient::subscribe(const char* topic, int qos){
    return esp_mqtt_client_subscribe(mqtt_client_, topic, qos);
}
