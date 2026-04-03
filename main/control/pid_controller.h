#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PID controller configuration
 */
typedef struct {
    float kp;                       /*!< Proportional gain */
    float ki;                       /*!< Integral gain */
    float kd;                       /*!< Derivative gain */
    float setpoint;                 /*!< Target value */
    float output_min;               /*!< Minimum output value */
    float output_max;               /*!< Maximum output value */
    float integral_min;             /*!< Minimum integral value (anti-windup) */
    float integral_max;             /*!< Maximum integral value (anti-windup) */
    float deadband;                 /*!< Deadband to prevent oscillation */
} pid_config_t;

/**
 * @brief PID controller state
 */
typedef struct {
    float integral;                 /*!< Integral term */
    float prev_error;               /*!< Previous error for derivative */
    float prev_measurement;         /*!< Previous measurement for derivative on measurement */
    float output;                   /*!< Current output */
    uint32_t last_time;             /*!< Last update time in microseconds */
} pid_state_t;

/**
 * @brief Initialize PID controller
 * 
 * @param config PID configuration
 * @param state Pointer to store PID state
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pid_controller_init(const pid_config_t *config, pid_state_t *state);

/**
 * @brief Update PID controller
 * 
 * @param state PID state
 * @param config PID configuration
 * @param measured Current measured value
 * @param output Pointer to store output
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pid_controller_update(pid_state_t *state, const pid_config_t *config, 
                                float measured, float *output);

/**
 * @brief Reset PID controller
 * 
 * @param state PID state to reset
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pid_controller_reset(pid_state_t *state);

/**
 * @brief Set PID setpoint
 * 
 * @param config PID configuration
 * @param setpoint New setpoint value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pid_controller_set_setpoint(pid_config_t *config, float setpoint);

/**
 * @brief Set PID gains
 * 
 * @param config PID configuration
 * @param kp Proportional gain
 * @param ki Integral gain
 * @param kd Derivative gain
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pid_controller_set_gains(pid_config_t *config, float kp, float ki, float kd);

/**
 * @brief Get PID output
 * 
 * @param state PID state
 * @return float Current output value
 */
float pid_controller_get_output(const pid_state_t *state);

#ifdef __cplusplus
}
#endif

#endif // PID_CONTROLLER_H
