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

  //set default handlers
  set_default_handlers();

  // Start the MQTT client
  esp_mqtt_client_start(mqtt_client_);

  ESP_LOGI("TAG", "MQTT client initialized and started");
}

// Static event handler required by the ESP-IDF
void MQTTClient::event_handler(void* arg, esp_event_base_t _, int32_t id, void* data) {
  auto* instance = static_cast<MQTTClient*>(arg);

  ESP_LOGD(TAG, "Received event id=%d", id);

  //check if event is connected or disconnected to update is_connected_ variable
  if(id == MQTT_EVENT_CONNECTED){
    instance->is_connected_.store(true);
  }
  else if(id == MQTT_EVENT_DISCONNECTED){
    instance->is_connected_.store(false);
  }

  instance->handle((esp_mqtt_event_handle_t)data);
}

// Instance-level event handler
void MQTTClient::handle(esp_mqtt_event_handle_t data) {
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

// Set default handlers for MQTT events
void MQTTClient::set_default_handlers() {
    // Triggered when the client successfully connects to the broker
    register_event_callback(MQTT_EVENT_CONNECTED, [this](auto _) {
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // Publish a test message
        int msg_id = esp_mqtt_client_publish(
            mqtt_client_, "/topic/test", "Hello from ESP32!",0, 1, 0
        );
        if (msg_id < 0)
            ESP_LOGE(TAG, "Failed to publish in '/topic/test', msg_id=%d", msg_id);
    });

    // Triggered when the client receives data from a subscribed topic
    register_event_callback(MQTT_EVENT_DATA, [this](auto data) {
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "Received topic: %.*s", data->topic_len, data->topic);
        ESP_LOGI(TAG, "Received data: %.*s", data->data_len, data->data);
    });
}
