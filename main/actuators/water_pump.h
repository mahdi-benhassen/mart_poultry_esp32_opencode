#ifndef WATER_PUMP_H
#define WATER_PUMP_H

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Water pump configuration
 */
typedef struct {
    gpio_num_t gpio_num;            /*!< GPIO pin for pump control */
    bool active_high;               /*!< Active high or low */
    float flow_rate_ml_per_sec;     /*!< Flow rate in ml per second */
} water_pump_config_t;

/**
 * @brief Initialize water pump
 * 
 * @param config Pump configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_pump_init(const water_pump_config_t *config);

/**
 * @brief Turn pump on
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_pump_on(void);

/**
 * @brief Turn pump off
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_pump_off(void);

/**
 * @brief Check if pump is running
 * 
 * @return true if pump is running
 */
bool water_pump_is_running(void);

/**
 * @brief Pump specific amount of water
 * 
 * @param ml Amount of water in milliliters
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_pump_dispense(float ml);

/**
 * @brief Set flow rate
 * 
 * @param rate Flow rate in ml per second
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_pump_set_flow_rate(float rate);

/**
 * @brief Deinitialize water pump
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_pump_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // WATER_PUMP_H
