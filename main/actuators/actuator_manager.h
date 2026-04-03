#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include "esp_err.h"
#include "../include/poultry_system_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all actuators
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_init(void);

/**
 * @brief Set all actuator states
 * 
 * @param states Pointer to actuator states
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_set_states(const actuator_states_t *states);

/**
 * @brief Get all actuator states
 * 
 * @param states Pointer to store actuator states
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_get_states(actuator_states_t *states);

/**
 * @brief Set exhaust fan speed
 * 
 * @param speed Fan speed (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_set_exhaust_fan(uint8_t speed);

/**
 * @brief Set inlet fan speed
 * 
 * @param speed Fan speed (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_set_inlet_fan(uint8_t speed);

/**
 * @brief Set heater power
 * 
 * @param power Heater power (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_set_heater(uint8_t power);

/**
 * @brief Set feeder speed
 * 
 * @param speed Feeder speed (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_set_feeder(uint8_t speed);

/**
 * @brief Set water pump state
 * 
 * @param state Pump state (0=off, 1=on)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_set_water_pump(uint8_t state);

/**
 * @brief Set lighting intensity
 * 
 * @param intensity Lighting intensity (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_set_lighting(uint8_t intensity);

/**
 * @brief Set ventilation position
 * 
 * @param position Ventilation position (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_set_ventilation(uint8_t position);

/**
 * @brief Set alarm state
 * 
 * @param state Alarm state (true=on, false=off)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_set_alarm(bool state);

/**
 * @brief Emergency stop all actuators
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_emergency_stop(void);

/**
 * @brief Deinitialize all actuators
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t actuator_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // ACTUATOR_MANAGER_H
