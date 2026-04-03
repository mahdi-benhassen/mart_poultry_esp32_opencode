#ifndef HEATER_CONTROL_H
#define HEATER_CONTROL_H

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Heater configuration
 */
typedef struct {
    gpio_num_t gpio_num;            /*!< GPIO pin for heater control */
    ledc_channel_t pwm_channel;     /*!< PWM channel */
    ledc_timer_t pwm_timer;         /*!< PWM timer */
    uint32_t pwm_frequency;         /*!< PWM frequency in Hz */
    ledc_timer_bit_t pwm_resolution; /*!< PWM resolution */
    bool inverted;                  /*!< Invert PWM signal */
    float max_temperature;          /*!< Maximum temperature limit */
} heater_config_t;

/**
 * @brief Initialize heater control
 * 
 * @param config Heater configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t heater_control_init(const heater_config_t *config);

/**
 * @brief Set heater power
 * 
 * @param power Heater power (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t heater_control_set_power(uint8_t power);

/**
 * @brief Get current heater power
 * 
 * @return uint8_t Current power (0-100%)
 */
uint8_t heater_control_get_power(void);

/**
 * @brief Turn heater on (100% power)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t heater_control_on(void);

/**
 * @brief Turn heater off (0% power)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t heater_control_off(void);

/**
 * @brief Check if heater is on
 * 
 * @return true if heater is on
 */
bool heater_control_is_on(void);

/**
 * @brief Set temperature limit
 * 
 * @param max_temp Maximum temperature in Celsius
 * @return esp_err_t ESP_OK on success
 */
esp_err_t heater_control_set_limit(float max_temp);

/**
 * @brief Deinitialize heater control
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t heater_control_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // HEATER_CONTROL_H
