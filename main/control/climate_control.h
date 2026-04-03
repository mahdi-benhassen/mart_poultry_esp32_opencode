#ifndef CLIMATE_CONTROL_H
#define CLIMATE_CONTROL_H

#include "esp_err.h"
#include "../include/poultry_system_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Climate control configuration
 */
typedef struct {
    float temp_setpoint;            /*!< Temperature setpoint in Celsius */
    float humidity_setpoint;        /*!< Humidity setpoint in percentage */
    float ammonia_limit;            /*!< Ammonia limit in PPM */
    float co2_limit;                /*!< CO2 limit in PPM */
    float temp_tolerance;           /*!< Temperature tolerance */
    float humidity_tolerance;       /*!< Humidity tolerance */
} climate_config_t;

/**
 * @brief Initialize climate control
 * 
 * @param config Climate configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t climate_control_init(const climate_config_t *config);

/**
 * @brief Update climate control based on sensor data
 * 
 * @param sensor_data Current sensor data
 * @param actuator_states Pointer to store actuator states
 * @return esp_err_t ESP_OK on success
 */
esp_err_t climate_control_update(const sensor_data_t *sensor_data, 
                                 actuator_states_t *actuator_states);

/**
 * @brief Set temperature setpoint
 * 
 * @param setpoint Temperature setpoint in Celsius
 * @return esp_err_t ESP_OK on success
 */
esp_err_t climate_control_set_temp_setpoint(float setpoint);

/**
 * @brief Set humidity setpoint
 * 
 * @param setpoint Humidity setpoint in percentage
 * @return esp_err_t ESP_OK on success
 */
esp_err_t climate_control_set_humidity_setpoint(float setpoint);

/**
 * @brief Set ammonia limit
 * 
 * @param limit Ammonia limit in PPM
 * @return esp_err_t ESP_OK on success
 */
esp_err_t climate_control_set_ammonia_limit(float limit);

/**
 * @brief Set CO2 limit
 * 
 * @param limit CO2 limit in PPM
 * @return esp_err_t ESP_OK on success
 */
esp_err_t climate_control_set_co2_limit(float limit);

/**
 * @brief Get current climate status
 * 
 * @param status Pointer to store status string
 * @param max_len Maximum length of status string
 * @return esp_err_t ESP_OK on success
 */
esp_err_t climate_control_get_status(char *status, size_t max_len);

/**
 * @brief Reset climate control
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t climate_control_reset(void);

/**
 * @brief Deinitialize climate control
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t climate_control_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // CLIMATE_CONTROL_H
