#include "ventilation_control.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "VENTILATION_CTRL";

static ventilation_config_t vent_config = {0};
static uint8_t current_position = 0;
static bool initialized = false;

esp_err_t ventilation_control_init(const ventilation_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&vent_config, config, sizeof(ventilation_config_t));
    
    // Set defaults if not specified
    if (vent_config.pwm_frequency == 0) {
        vent_config.pwm_frequency = 50; // 50Hz for servo
    }
    if (vent_config.pwm_resolution == 0) {
        vent_config.pwm_resolution = LEDC_TIMER_16_BIT;
    }
    if (vent_config.min_pulse_us == 0) {
        vent_config.min_pulse_us = 500; // 0.5ms
    }
    if (vent_config.max_pulse_us == 0) {
        vent_config.max_pulse_us = 2500; // 2.5ms
    }
    
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = vent_config.pwm_resolution,
        .timer_num = vent_config.pwm_timer,
        .freq_hz = vent_config.pwm_frequency,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer");
        return ret;
    }
    
    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = vent_config.servo_gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = vent_config.pwm_channel,
        .timer_sel = vent_config.pwm_timer,
        .duty = 0,
        .hpoint = 0,
        .intr_type = LEDC_INTR_DISABLE
    };
    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel");
        return ret;
    }
    
    // Initialize to closed position
    current_position = 0;
    initialized = true;
    
    ESP_LOGI(TAG, "Ventilation control initialized on GPIO %d, PWM channel %d", 
             vent_config.servo_gpio, vent_config.pwm_channel);
    
    return ESP_OK;
}

esp_err_t ventilation_control_set_position(uint8_t position) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (position > 100) {
        position = 100;
    }
    
    // Calculate pulse width in microseconds
    uint16_t pulse_us = vent_config.min_pulse_us + 
                       ((vent_config.max_pulse_us - vent_config.min_pulse_us) * position / 100);
    
    // Calculate duty cycle for 16-bit resolution at 50Hz
    // Period = 20ms = 20000us
    // Duty = (pulse_us / 20000) * 65536
    uint32_t duty = (pulse_us * 65536) / 20000;
    
    // Set duty cycle
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, vent_config.pwm_channel, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty cycle");
        return ret;
    }
    
    // Update duty
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, vent_config.pwm_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update duty cycle");
        return ret;
    }
    
    current_position = position;
    ESP_LOGD(TAG, "Ventilation position set to %d%% (pulse: %dus)", position, pulse_us);
    
    return ESP_OK;
}

uint8_t ventilation_control_get_position(void) {
    return current_position;
}

esp_err_t ventilation_control_open(void) {
    return ventilation_control_set_position(100);
}

esp_err_t ventilation_control_close(void) {
    return ventilation_control_set_position(0);
}

bool ventilation_control_is_open(void) {
    return (current_position > 0);
}

esp_err_t ventilation_control_deinit(void) {
    if (initialized) {
        ledc_stop(LEDC_LOW_SPEED_MODE, vent_config.pwm_channel, 0);
        initialized = false;
        current_position = 0;
        ESP_LOGI(TAG, "Ventilation control deinitialized");
    }
    return ESP_OK;
}
