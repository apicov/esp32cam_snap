#include "MQTTClient.hpp"

MQTTClient::MQTTClient(const char* mqtt_broker_uri)
    :mqtt_broker_uri_(mqtt_broker_uri) {

    ESP_LOGI(TAG, "Initializing with %s", mqtt_broker_uri_);

    // MQTT configuration
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = mqtt_broker_uri_;  // Correct URI assignment
    mqtt_cfg.session.keepalive = 10;  // Set the keep-alive interval

    // Initialize MQTT client
    if (!(mqtt_client_= esp_mqtt_client_init(&mqtt_cfg))) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    // Register the handler for the supported events
    for (const esp_mqtt_event_id_t& e:{
            MQTT_EVENT_CONNECTED,
            MQTT_EVENT_DISCONNECTED,
            MQTT_EVENT_DATA })
        esp_mqtt_client_register_event(mqtt_client_, e, event_handler, this);


    // Start the MQTT client
    esp_mqtt_client_start(mqtt_client_);
    ESP_LOGI(TAG, "Initialized");
}

// Static event handler required by the ESP-IDF
void MQTTClient::event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    ESP_LOGD(TAG, "Received event id=%d", id);
    static_cast<MQTTClient*>(arg)->handle(base, id, (esp_mqtt_event_handle_t)data);
}

// Instance-level event handler
void MQTTClient::handle(esp_event_base_t base, int32_t id, esp_mqtt_event_handle_t data) {
    if(id == MQTT_EVENT_CONNECTED) {
        is_connected_.store(true);
        ESP_LOGI(TAG, "Connected to broker");

        // XXX: Publishing a test message should be an optional feature,
        // which could be enabled by the user at compile time
        ESP_LOGI(TAG, "Sending a test message");
        publish("/topic/test", "Hello from ESP32!", 1, 0);
        for (const auto& f: on_connect_cb) f(data);
    }
    else if (id == MQTT_EVENT_DISCONNECTED) {
        is_connected_.store(false);
        ESP_LOGI(TAG, "The client is disconnected");
        for (const auto& f: on_disconnect_cb) f(data);
    }
    else if (id == MQTT_EVENT_DATA) {
        ESP_LOGI(TAG, "Incoming data");
        for (const auto& f: on_data_received_cb) f(data);
    }
}

MQTTClient& MQTTClient::on_connect(MQTTEventCallback f) {
    on_connect_cb.push_back(f);
    return *this;
}
MQTTClient& MQTTClient::on_disconnect(MQTTEventCallback f) {
    on_disconnect_cb.push_back(f);
    return *this;
}
MQTTClient& MQTTClient::on_data_received(MQTTEventCallback f){
    on_data_received_cb.push_back(f);
    return *this;
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
        ESP_LOGI(TAG, "The message was published with id=%d", msg_id);
    } else {
        ESP_LOGE(TAG, "Failed to publish message");
    }
}

esp_err_t MQTTClient::subscribe(const char* topic, int qos){
    return esp_mqtt_client_subscribe(mqtt_client_, topic, qos);
}

esp_err_t MQTTClient::reconnect() {
    return esp_mqtt_client_reconnect(mqtt_client_);
}
