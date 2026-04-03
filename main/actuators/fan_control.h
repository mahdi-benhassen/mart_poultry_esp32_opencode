#ifndef FAN_CONTROL_H
#define FAN_CONTROL_H

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fan configuration
 */
typedef struct {
    gpio_num_t gpio_num;            /*!< GPIO pin for fan control */
    ledc_channel_t pwm_channel;     /*!< PWM channel */
    ledc_timer_t pwm_timer;         /*!< PWM timer */
    uint32_t pwm_frequency;         /*!< PWM frequency in Hz */
    ledc_timer_bit_t pwm_resolution; /*!< PWM resolution */
    bool inverted;                  /*!< Invert PWM signal */
} fan_config_t;

/**
 * @brief Initialize fan control
 * 
 * @param config Fan configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fan_control_init(const fan_config_t *config);

/**
 * @brief Set fan speed
 * 
 * @param speed Fan speed (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fan_control_set_speed(uint8_t speed);

/**
 * @brief Get current fan speed
 * 
 * @return uint8_t Current speed (0-100%)
 */
uint8_t fan_control_get_speed(void);

/**
 * @brief Turn fan on (100% speed)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fan_control_on(void);

/**
 * @brief Turn fan off (0% speed)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fan_control_off(void);

/**
 * @brief Check if fan is running
 * 
 * @return true if fan is running
 */
bool fan_control_is_running(void);

/**
 * @brief Deinitialize fan control
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t fan_control_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // FAN_CONTROL_H
