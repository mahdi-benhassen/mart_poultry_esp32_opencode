#ifndef POULTRY_MQTT_H
#define POULTRY_MQTT_H

#include "esp_err.h"
#include <mqtt_client.h>
#include "../include/poultry_system_config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT client configuration
 */
typedef struct {
    char broker[MQTT_BROKER_MAX_LEN];   /*!< MQTT broker URL */
    uint16_t port;                      /*!< MQTT broker port */
    char username[32];                  /*!< MQTT username */
    char password[32];                  /*!< MQTT password */
    char client_id[MQTT_CLIENT_ID_MAX_LEN]; /*!< MQTT client ID */
    uint16_t keepalive;                 /*!< Keepalive interval in seconds */
    uint8_t qos;                        /*!< Quality of Service (0, 1, or 2) */
    bool clean_session;                 /*!< Clean session flag */
} mqtt_config_t;

/**
 * @brief MQTT connection status
 */
typedef enum {
    MQTT_STATUS_DISCONNECTED = 0,       /*!< Disconnected */
    MQTT_STATUS_CONNECTING,             /*!< Connecting */
    MQTT_STATUS_CONNECTED,              /*!< Connected */
    MQTT_STATUS_ERROR                   /*!< Error */
} mqtt_status_t;

/**
 * @brief Initialize MQTT client
 * 
 * @param config MQTT configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_init(const mqtt_config_t *config);

/**
 * @brief Connect to MQTT broker
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_connect(void);

/**
 * @brief Disconnect from MQTT broker
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_disconnect(void);

/**
 * @brief Publish message to topic
 * 
 * @param topic MQTT topic
 * @param data Message data
 * @param len Message length
 * @param qos Quality of Service (0, 1, or 2)
 * @param retain Retain flag
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_publish(const char *topic, const char *data, size_t len,
                              uint8_t qos, bool retain);

/**
 * @brief Subscribe to topic
 * 
 * @param topic MQTT topic
 * @param qos Quality of Service (0, 1, or 2)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_subscribe(const char *topic, uint8_t qos);

/**
 * @brief Unsubscribe from topic
 * 
 * @param topic MQTT topic
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_unsubscribe(const char *topic);

/**
 * @brief Get MQTT status
 * 
 * @return mqtt_status_t Current MQTT status
 */
mqtt_status_t mqtt_client_get_status(void);

/**
 * @brief Check if connected to MQTT broker
 * 
 * @return true if connected
 */
bool mqtt_client_is_connected(void);

/**
 * @brief Publish sensor data
 * 
 * @param sensor_data Sensor data to publish
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_publish_sensor_data(const sensor_data_t *sensor_data);

/**
 * @brief Publish actuator states
 * 
 * @param actuator_states Actuator states to publish
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_publish_actuator_states(const actuator_states_t *actuator_states);

/**
 * @brief Publish alert
 * 
 * @param alert_type Alert type
 * @param message Alert message
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_publish_alert(alert_type_t alert_type, const char *message);

/**
 * @brief Set MQTT configuration
 * 
 * @param config MQTT configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_set_config(const mqtt_config_t *config);

/**
 * @brief Get MQTT configuration
 * 
 * @param config Pointer to store MQTT configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_get_config(mqtt_config_t *config);

/**
 * @brief Deinitialize MQTT client
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_client_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // POULTRY_MQTT_H
