#ifndef ALERT_SYSTEM_H
#define ALERT_SYSTEM_H

#include "esp_err.h"
#include "../include/poultry_system_config.h"
#include "../connectivity/mqtt_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Alert configuration
 */
typedef struct {
    bool enabled;                   /*!< Enable/disable alerts */
    bool mqtt_enabled;              /*!< Enable MQTT alerts */
    bool alarm_enabled;             /*!< Enable local alarm */
    uint32_t cooldown_seconds;      /*!< Cooldown between same alerts */
} alert_config_t;

/**
 * @brief Alert event structure
 */
typedef struct {
    alert_type_t type;              /*!< Alert type */
    uint32_t timestamp;             /*!< Alert timestamp */
    float value;                    /*!< Current value that triggered alert */
    float threshold;                /*!< Threshold value */
    char message[128];              /*!< Alert message */
} alert_event_t;

/**
 * @brief Initialize alert system
 * 
 * @param config Alert configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t alert_system_init(const alert_config_t *config);

/**
 * @brief Check sensor data and trigger alerts if needed
 * 
 * @param sensor_data Current sensor data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t alert_system_check(const sensor_data_t *sensor_data);

/**
 * @brief Trigger alert manually
 * 
 * @param type Alert type
 * @param value Current value
 * @param threshold Threshold value
 * @param message Alert message
 * @return esp_err_t ESP_OK on success
 */
esp_err_t alert_system_trigger(alert_type_t type, float value, float threshold, 
                               const char *message);

/**
 * @brief Get active alert
 * 
 * @return alert_type_t Active alert type
 */
alert_type_t alert_system_get_active(void);

/**
 * @brief Clear active alert
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t alert_system_clear(void);

/**
 * @brief Get alert history
 * 
 * @param history Array to store alert history
 * @param max_count Maximum number of alerts to retrieve
 * @param count Pointer to store actual number of alerts retrieved
 * @return esp_err_t ESP_OK on success
 */
esp_err_t alert_system_get_history(alert_event_t *history, size_t max_count, 
                                   size_t *count);

/**
 * @brief Enable/disable alert system
 * 
 * @param enabled true to enable, false to disable
 * @return esp_err_t ESP_OK on success
 */
esp_err_t alert_system_enable(bool enabled);

/**
 * @brief Check if alert system is enabled
 * 
 * @return true if enabled
 */
bool alert_system_is_enabled(void);

/**
 * @brief Set alert thresholds
 * 
 * @param temp_high High temperature threshold
 * @param temp_low Low temperature threshold
 * @param humidity_high High humidity threshold
 * @param humidity_low Low humidity threshold
 * @param ammonia_high High ammonia threshold
 * @param co2_high High CO2 threshold
 * @param water_low Low water level threshold
 * @param feed_low Low feed level threshold
 * @return esp_err_t ESP_OK on success
 */
esp_err_t alert_system_set_thresholds(float temp_high, float temp_low,
                                      float humidity_high, float humidity_low,
                                      float ammonia_high, float co2_high,
                                      float water_low, float feed_low);

/**
 * @brief Set gas leak threshold
 *
 * @param gas_threshold Gas leak threshold in ppm
 * @return esp_err_t ESP_OK on success
 */
esp_err_t alert_system_set_gas_threshold(float gas_threshold);

/**
 * @brief Deinitialize alert system
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t alert_system_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // ALERT_SYSTEM_H
