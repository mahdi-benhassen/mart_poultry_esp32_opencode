#include "poultry_mqtt.h"
#include <string.h>
#include <stdio.h>
static const char *TAG = "MQTT_CLIENT";

static mqtt_config_t mqtt_config = {0};
static mqtt_status_t mqtt_status = MQTT_STATUS_DISCONNECTED;
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool initialized = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            mqtt_status = MQTT_STATUS_CONNECTED;
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");
            mqtt_status = MQTT_STATUS_DISCONNECTED;
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT subscribed, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT unsubscribed, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT published, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT data received");
            ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Data: %.*s", event->data_len, event->data);
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            mqtt_status = MQTT_STATUS_ERROR;
            break;
            
        default:
            ESP_LOGD(TAG, "MQTT event id: %d", event->event_id);
            break;
    }
}

esp_err_t mqtt_client_init(const mqtt_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&mqtt_config, config, sizeof(mqtt_config_t));
    
    // Set defaults
    if (mqtt_config.port == 0) {
        mqtt_config.port = 1883;
    }
    if (mqtt_config.keepalive == 0) {
        mqtt_config.keepalive = MQTT_KEEPALIVE;
    }
    
    // Configure MQTT client
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_config.broker,
        .port = mqtt_config.port,
        .username = mqtt_config.username,
        .password = mqtt_config.password,
        .client_id = mqtt_config.client_id,
        .keepalive = mqtt_config.keepalive,
        .clean_session = mqtt_config.clean_session,
        .disable_auto_reconnect = false,
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }
    
    // Register event handler
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, 
                                   mqtt_event_handler, NULL);
    
    initialized = true;
    ESP_LOGI(TAG, "MQTT client initialized: broker=%s, port=%d", 
             mqtt_config.broker, mqtt_config.port);
    
    return ESP_OK;
}

esp_err_t mqtt_client_connect(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Connecting to MQTT broker: %s", mqtt_config.broker);
    mqtt_status = MQTT_STATUS_CONNECTING;
    
    esp_err_t ret = esp_mqtt_client_start(mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        mqtt_status = MQTT_STATUS_ERROR;
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t mqtt_client_disconnect(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = esp_mqtt_client_stop(mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop MQTT client");
        return ret;
    }
    
    mqtt_status = MQTT_STATUS_DISCONNECTED;
    ESP_LOGI(TAG, "Disconnected from MQTT broker");
    
    return ESP_OK;
}

esp_err_t mqtt_client_publish(const char *topic, const char *data, size_t len,
                              uint8_t qos, bool retain) {
    if (!initialized || topic == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (mqtt_status != MQTT_STATUS_CONNECTED) {
        ESP_LOGW(TAG, "MQTT not connected, cannot publish");
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, retain);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish to topic: %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGD(TAG, "Published to topic: %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

esp_err_t mqtt_client_subscribe(const char *topic, uint8_t qos) {
    if (!initialized || topic == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (mqtt_status != MQTT_STATUS_CONNECTED) {
        ESP_LOGW(TAG, "MQTT not connected, cannot subscribe");
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, qos);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to subscribe to topic: %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Subscribed to topic: %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

esp_err_t mqtt_client_unsubscribe(const char *topic) {
    if (!initialized || topic == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (mqtt_status != MQTT_STATUS_CONNECTED) {
        ESP_LOGW(TAG, "MQTT not connected, cannot unsubscribe");
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_unsubscribe(mqtt_client, topic);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to unsubscribe from topic: %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Unsubscribed from topic: %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

mqtt_status_t mqtt_client_get_status(void) {
    return mqtt_status;
}

bool mqtt_client_is_connected(void) {
    return (mqtt_status == MQTT_STATUS_CONNECTED);
}

esp_err_t mqtt_client_publish_sensor_data(const sensor_data_t *sensor_data) {
    if (!initialized || sensor_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Create JSON payload
    char json_buffer[512];
    snprintf(json_buffer, sizeof(json_buffer),
             "{\"temperature\":%.2f,\"humidity\":%.2f,\"ammonia\":%.2f,"
             "\"co2\":%.2f,\"light\":%.2f,\"water_level\":%.2f,"
             "\"feed_level\":%.2f,\"timestamp\":%u}",
             sensor_data->temperature, sensor_data->humidity,
             sensor_data->ammonia_ppm, sensor_data->co2_ppm,
             sensor_data->light_lux, sensor_data->water_level_percent,
             sensor_data->feed_level_percent, sensor_data->timestamp);
    
    // Publish to sensor data topic
    char topic[64];
    snprintf(topic, sizeof(topic), "poultry/%s/sensors", mqtt_config.client_id);
    
    return mqtt_client_publish(topic, json_buffer, strlen(json_buffer), 
                              mqtt_config.qos, false);
}

esp_err_t mqtt_client_publish_actuator_states(const actuator_states_t *actuator_states) {
    if (!initialized || actuator_states == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Create JSON payload
    char json_buffer[512];
    snprintf(json_buffer, sizeof(json_buffer),
             "{\"exhaust_fan\":%u,\"inlet_fan\":%u,\"heater\":%u,"
             "\"feeder\":%u,\"water_pump\":%u,\"lighting\":%u,"
             "\"ventilation\":%u,\"alarm\":%s}",
             actuator_states->exhaust_fan_speed, actuator_states->inlet_fan_speed,
             actuator_states->heater_power, actuator_states->feeder_speed,
             actuator_states->water_pump_state, actuator_states->lighting_intensity,
             actuator_states->ventilation_position,
             actuator_states->alarm_state ? "true" : "false");
    
    // Publish to actuator states topic
    char topic[64];
    snprintf(topic, sizeof(topic), "poultry/%s/actuators", mqtt_config.client_id);
    
    return mqtt_client_publish(topic, json_buffer, strlen(json_buffer), 
                              mqtt_config.qos, false);
}

esp_err_t mqtt_client_publish_alert(alert_type_t alert_type, const char *message) {
    if (!initialized || message == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char escaped_message[256];
    size_t j = 0;
    for (size_t i = 0; message[i] != '\0' && j < sizeof(escaped_message) - 1; i++) {
        if (message[i] == '"' || message[i] == '\\') {
            if (j < sizeof(escaped_message) - 2) {
                escaped_message[j++] = '\\';
            }
        }
        if (message[i] >= 0x20 || message[i] == '\n' || message[i] == '\r' || message[i] == '\t') {
            escaped_message[j++] = message[i];
        }
    }
    escaped_message[j] = '\0';
    
    char json_buffer[256];
    snprintf(json_buffer, sizeof(json_buffer),
             "{\"alert_type\":%u,\"message\":\"%s\",\"timestamp\":%u}",
             alert_type, escaped_message, (uint32_t)(esp_timer_get_time() / 1000000));
    
    // Publish to alerts topic
    char topic[64];
    snprintf(topic, sizeof(topic), "poultry/%s/alerts", mqtt_config.client_id);
    
    return mqtt_client_publish(topic, json_buffer, strlen(json_buffer), 
                              mqtt_config.qos, false);
}

esp_err_t mqtt_client_set_config(const mqtt_config_t *config) {
    if (!initialized || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&mqtt_config, config, sizeof(mqtt_config_t));
    
    // Reinitialize MQTT client with new config
    if (mqtt_client != NULL) {
        esp_mqtt_client_destroy(mqtt_client);
    }
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_config.broker,
        .port = mqtt_config.port,
        .username = mqtt_config.username,
        .password = mqtt_config.password,
        .client_id = mqtt_config.client_id,
        .keepalive = mqtt_config.keepalive,
        .clean_session = mqtt_config.clean_session,
        .disable_auto_reconnect = false,
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to reinitialize MQTT client");
        return ESP_FAIL;
    }
    
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, 
                                   mqtt_event_handler, NULL);
    
    ESP_LOGI(TAG, "MQTT configuration updated");
    return ESP_OK;
}

esp_err_t mqtt_client_get_config(mqtt_config_t *config) {
    if (!initialized || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &mqtt_config, sizeof(mqtt_config_t));
    return ESP_OK;
}

esp_err_t mqtt_client_deinit(void) {
    if (initialized) {
        if (mqtt_client != NULL) {
            esp_mqtt_client_stop(mqtt_client);
            esp_mqtt_client_destroy(mqtt_client);
            mqtt_client = NULL;
        }
        initialized = false;
        mqtt_status = MQTT_STATUS_DISCONNECTED;
        ESP_LOGI(TAG, "MQTT client deinitialized");
    }
    return ESP_OK;
}
