#ifndef FEEDER_CONTROL_H
#define FEEDER_CONTROL_H

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Feeder configuration
 */
typedef struct {
    gpio_num_t gpio_num;            /*!< GPIO pin for feeder motor */
    ledc_channel_t pwm_channel;     /*!< PWM channel */
    ledc_timer_t pwm_timer;         /*!< PWM timer */
    uint32_t pwm_frequency;         /*!< PWM frequency in Hz */
    ledc_timer_bit_t pwm_resolution; /*!< PWM resolution */
    bool inverted;                  /*!< Invert PWM signal */
    float feed_rate_g_per_sec;      /*!< Feed rate in grams per second */
} feeder_config_t;

/**
 * @brief Initialize feeder control
 * 
 * @param config Feeder configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeder_control_init(const feeder_config_t *config);

/**
 * @brief Set feeder speed
 * 
 * @param speed Feeder speed (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeder_control_set_speed(uint8_t speed);

/**
 * @brief Get current feeder speed
 * 
 * @return uint8_t Current speed (0-100%)
 */
uint8_t feeder_control_get_speed(void);

/**
 * @brief Dispense specific amount of feed
 * 
 * @param grams Amount of feed in grams
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeder_control_dispense(float grams);

/**
 * @brief Start continuous feeding
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeder_control_start(void);

/**
 * @brief Stop feeding
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeder_control_stop(void);

/**
 * @brief Check if feeder is running
 * 
 * @return true if feeder is running
 */
bool feeder_control_is_running(void);

/**
 * @brief Set feed rate
 * 
 * @param rate Feed rate in grams per second
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeder_control_set_rate(float rate);

/**
 * @brief Deinitialize feeder control
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeder_control_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // FEEDER_CONTROL_H
