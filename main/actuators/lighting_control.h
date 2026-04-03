#ifndef LIGHTING_CONTROL_H
#define LIGHTING_CONTROL_H

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lighting configuration
 */
typedef struct {
    gpio_num_t gpio_num;            /*!< GPIO pin for lighting */
    ledc_channel_t pwm_channel;     /*!< PWM channel */
    ledc_timer_t pwm_timer;         /*!< PWM timer */
    uint32_t pwm_frequency;         /*!< PWM frequency in Hz */
    ledc_timer_bit_t pwm_resolution; /*!< PWM resolution */
    bool inverted;                  /*!< Invert PWM signal */
} lighting_config_t;

/**
 * @brief Initialize lighting control
 * 
 * @param config Lighting configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_control_init(const lighting_config_t *config);

/**
 * @brief Set lighting intensity
 * 
 * @param intensity Lighting intensity (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_control_set_intensity(uint8_t intensity);

/**
 * @brief Get current lighting intensity
 * 
 * @return uint8_t Current intensity (0-100%)
 */
uint8_t lighting_control_get_intensity(void);

/**
 * @brief Turn lights on (100% intensity)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_control_on(void);

/**
 * @brief Turn lights off (0% intensity)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_control_off(void);

/**
 * @brief Check if lights are on
 * 
 * @return true if lights are on
 */
bool lighting_control_is_on(void);

/**
 * @brief Set sunrise simulation
 * 
 * @param duration_minutes Duration of sunrise in minutes
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_control_sunrise(uint16_t duration_minutes);

/**
 * @brief Set sunset simulation
 * 
 * @param duration_minutes Duration of sunset in minutes
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_control_sunset(uint16_t duration_minutes);

/**
 * @brief Deinitialize lighting control
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_control_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // LIGHTING_CONTROL_H
