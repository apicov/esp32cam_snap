#include "MQTTClient.hpp"
    
MQTTClient::MQTTClient(const char* mqtt_broker_uri)
  :mqtt_broker_uri_(mqtt_broker_uri){}
  
 void MQTTClient::init(){

  // MQTT configuration                                                                      
  esp_mqtt_client_config_t mqtt_cfg = {};
  mqtt_cfg.broker.address.uri = mqtt_broker_uri_;  // Correct URI assignment
  mqtt_cfg.session.keepalive = 10;  // Set the keep-alive interval 

  ESP_LOGI(TAG, "Initializing MQTT client with URI: %s", mqtt_broker_uri_);
                                                                                             
  // Initialize MQTT client                                                                
  mqtt_client_ = esp_mqtt_client_init(&mqtt_cfg);                                             
  if (mqtt_client_ == NULL) {
      ESP_LOGE(TAG, "Failed to initialize MQTT client");
      return;
  }
                                                                                             
  // Register the event handler                                                              
  esp_mqtt_client_register_event(mqtt_client_, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), &MQTTClient::event_handler_static, this);

  //set default handlers
  set_default_handlers();
                                                                                             
  // Start the MQTT client                                                                   
  esp_mqtt_client_start(mqtt_client_);                                                          
                                                                                             
  ESP_LOGI("TAG", "MQTT client initialized and started");                                      
}

// Static event handler required by the ESP-IDF
void MQTTClient::event_handler_static(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  auto* instance = static_cast<MQTTClient*>(arg);
  instance->event_handler(event_data);
}


void MQTTClient::register_event_callback(int32_t event_id, MQTTEventCallback callback) {
    event_callbacks_[event_id] = std::move(callback);
}

bool MQTTClient::is_connected() const {
    return is_connected_.load();
}


void MQTTClient::publish(const char* topic, const char* data, int qos, int retain) {
    // Check if the MQTT client is connected before attempting to publish
    if (!is_connected_.load()) {
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


// Instance-level event handler
void MQTTClient::event_handler(void* event_data) {
    auto event = (esp_mqtt_event_handle_t) event_data;
    auto key = event->event_id;
    auto it = event_callbacks_.find(key);
    
    if (it != event_callbacks_.end()) {
        // Call the registered callback for this event
        it->second(event_data);
    } else {
        ESP_LOGW(TAG, "Unhandled event: id=%d", event->event_id );
    }
}  

  
// Set default handlers for MQTT events
void MQTTClient::set_default_handlers() {
    // Triggered when the client successfully connects to the broker
    register_event_callback(MQTT_EVENT_CONNECTED, [this](void* event_data) {
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        is_connected_.store(true);
        // Publish a message after connection
        esp_mqtt_client_publish(mqtt_client_, "/topic/test", "Hello from ESP32!", 0, 1, 0);
    });

    // Triggered when the client disconnects from the broker
    register_event_callback(MQTT_EVENT_DISCONNECTED, [this](void* event_data) {
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        is_connected_.store(false);
    });

    // Triggered when there is an error in the MQTT client
    register_event_callback(MQTT_EVENT_ERROR, [this](void* event_data) {
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
    });

    // Triggered when a message is successfully published
    register_event_callback(MQTT_EVENT_PUBLISHED, [this](void* event_data) {
        auto event = (esp_mqtt_event_handle_t)event_data;
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    });

    // Triggered when the client successfully subscribes to a topic
    register_event_callback(MQTT_EVENT_SUBSCRIBED, [this](void* event_data) {
        auto event = (esp_mqtt_event_handle_t)event_data;
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    });

    // Triggered when the client successfully unsubscribes from a topic
    register_event_callback(MQTT_EVENT_UNSUBSCRIBED, [this](void* event_data) {
        auto event = (esp_mqtt_event_handle_t)event_data;
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    });

    // Triggered when the client receives data from a subscribed topic
    register_event_callback(MQTT_EVENT_DATA, [this](void* event_data) {
        auto event = (esp_mqtt_event_handle_t)event_data;
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "Received topic: %.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "Received data: %.*s", event->data_len, event->data);
    });

    // Triggered before the client attempts to connect to the broker
    register_event_callback(MQTT_EVENT_BEFORE_CONNECT, [this](void* event_data) {
        ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
    });

    // Triggered when the client is deleted
    register_event_callback(MQTT_EVENT_DELETED, [this](void* event_data) {
        ESP_LOGI(TAG, "MQTT_EVENT_DELETED");
    });
}

