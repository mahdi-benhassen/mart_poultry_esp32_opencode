#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "esp_err.h"
#include "../include/poultry_system_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all sensors
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_manager_init(void);

/**
 * @brief Read all sensor data
 * 
 * @param data Pointer to store sensor data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_manager_read_all(sensor_data_t *data);

/**
 * @brief Read specific sensor
 * 
 * @param sensor_type Sensor type to read
 * @param value Pointer to store value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_manager_read_sensor(uint8_t sensor_type, float *value);

/**
 * @brief Check if all sensors are connected
 * 
 * @return true if all sensors are responding
 */
bool sensor_manager_check_all_connected(void);

/**
 * @brief Get sensor health status
 * 
 * @param sensor_mask Pointer to store sensor health bitmask
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_manager_get_health(uint8_t *sensor_mask);

/**
 * @brief Calibrate specific sensor
 * 
 * @param sensor_type Sensor type to calibrate
 * @param known_value Known calibration value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_manager_calibrate(uint8_t sensor_type, float known_value);

/**
 * @brief Deinitialize all sensors
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_MANAGER_H
