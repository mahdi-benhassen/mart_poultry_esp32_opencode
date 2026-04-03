#include "pid_controller.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <math.h>

static const char *TAG = "PID_CONTROLLER";

esp_err_t pid_controller_init(const pid_config_t *config, pid_state_t *state) {
    if (config == NULL || state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(state, 0, sizeof(pid_state_t));
    state->last_time = esp_timer_get_time();
    state->prev_measurement = 0;
    
    ESP_LOGI(TAG, "PID controller initialized: Kp=%.2f, Ki=%.2f, Kd=%.2f, Setpoint=%.2f",
             config->kp, config->ki, config->kd, config->setpoint);
    
    return ESP_OK;
}

esp_err_t pid_controller_update(pid_state_t *state, const pid_config_t *config, 
                                float measured, float *output) {
    if (state == NULL || config == NULL || output == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint32_t current_time = esp_timer_get_time();
    float dt = (current_time - state->last_time) / 1000000.0f;
    
    if (dt <= 0) {
        dt = 0.001f;
    }
    
    float error = config->setpoint - measured;
    
    if (config->deadband > 0 && fabsf(error) < config->deadband) {
        error = 0;
    }
    
    float p_term = config->kp * error;
    
    state->integral += error * dt;
    
    if (state->integral > config->integral_max) {
        state->integral = config->integral_max;
    } else if (state->integral < config->integral_min) {
        state->integral = config->integral_min;
    }
    
    float i_term = config->ki * state->integral;
    
    float derivative = -(measured - state->prev_measurement) / dt;
    float d_term = config->kd * derivative;
    
    float raw_output = p_term + i_term + d_term;
    
    if (raw_output > config->output_max) {
        raw_output = config->output_max;
    } else if (raw_output < config->output_min) {
        raw_output = config->output_min;
    }
    
    state->prev_error = error;
    state->prev_measurement = measured;
    state->output = raw_output;
    state->last_time = current_time;
    
    *output = raw_output;
    
    ESP_LOGD(TAG, "PID: Error=%.2f, P=%.2f, I=%.2f, D=%.2f, Output=%.2f",
             error, p_term, i_term, d_term, raw_output);
    
    return ESP_OK;
}

esp_err_t pid_controller_reset(pid_state_t *state) {
    if (state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    state->integral = 0;
    state->prev_error = 0;
    state->prev_measurement = 0;
    state->output = 0;
    state->last_time = esp_timer_get_time();
    
    ESP_LOGI(TAG, "PID controller reset");
    
    return ESP_OK;
}

esp_err_t pid_controller_set_setpoint(pid_config_t *config, float setpoint) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    config->setpoint = setpoint;
    ESP_LOGI(TAG, "PID setpoint set to %.2f", setpoint);
    
    return ESP_OK;
}

esp_err_t pid_controller_set_gains(pid_config_t *config, float kp, float ki, float kd) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    config->kp = kp;
    config->ki = ki;
    config->kd = kd;
    
    ESP_LOGI(TAG, "PID gains set: Kp=%.2f, Ki=%.2f, Kd=%.2f", kp, ki, kd);
    
    return ESP_OK;
}

float pid_controller_get_output(const pid_state_t *state) {
    if (state == NULL) {
        return 0;
    }
    return state->output;
}
