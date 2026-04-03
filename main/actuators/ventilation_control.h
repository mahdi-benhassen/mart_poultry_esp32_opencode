#ifndef VENTILATION_CONTROL_H
#define VENTILATION_CONTROL_H

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ventilation configuration
 */
typedef struct {
    gpio_num_t servo_gpio;          /*!< GPIO pin for servo motor */
    ledc_channel_t pwm_channel;     /*!< PWM channel */
    ledc_timer_t pwm_timer;         /*!< PWM timer */
    uint32_t pwm_frequency;         /*!< PWM frequency in Hz */
    ledc_timer_bit_t pwm_resolution; /*!< PWM resolution */
    uint16_t min_pulse_us;          /*!< Minimum pulse width in microseconds */
    uint16_t max_pulse_us;          /*!< Maximum pulse width in microseconds */
} ventilation_config_t;

/**
 * @brief Initialize ventilation control
 * 
 * @param config Ventilation configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ventilation_control_init(const ventilation_config_t *config);

/**
 * @brief Set ventilation position
 * 
 * @param position Position (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ventilation_control_set_position(uint8_t position);

/**
 * @brief Get current ventilation position
 * 
 * @return uint8_t Current position (0-100%)
 */
uint8_t ventilation_control_get_position(void);

/**
 * @brief Open ventilation fully
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ventilation_control_open(void);

/**
 * @brief Close ventilation fully
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ventilation_control_close(void);

/**
 * @brief Check if ventilation is open
 * 
 * @return true if ventilation is open
 */
bool ventilation_control_is_open(void);

/**
 * @brief Deinitialize ventilation control
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ventilation_control_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // VENTILATION_CONTROL_H
