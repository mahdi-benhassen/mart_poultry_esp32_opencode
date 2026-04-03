#include "fan_control.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "FAN_CONTROL";

static fan_config_t fan_config = {0};
static uint8_t current_speed = 0;
static bool initialized = false;

esp_err_t fan_control_init(const fan_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&fan_config, config, sizeof(fan_config_t));
    
    // Set defaults if not specified
    if (fan_config.pwm_frequency == 0) {
        fan_config.pwm_frequency = PWM_FREQUENCY;
    }
    if (fan_config.pwm_resolution == 0) {
        fan_config.pwm_resolution = PWM_RESOLUTION;
    }
    
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = fan_config.pwm_resolution,
        .timer_num = fan_config.pwm_timer,
        .freq_hz = fan_config.pwm_frequency,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer");
        return ret;
    }
    
    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = fan_config.gpio_num,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = fan_config.pwm_channel,
        .timer_sel = fan_config.pwm_timer,
        .duty = 0,
        .hpoint = 0,
        .intr_type = LEDC_INTR_DISABLE
    };
    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel");
        return ret;
    }
    
    // Initialize to off
    current_speed = 0;
    initialized = true;
    
    ESP_LOGI(TAG, "Fan control initialized on GPIO %d, PWM channel %d", 
             fan_config.gpio_num, fan_config.pwm_channel);
    
    return ESP_OK;
}

esp_err_t fan_control_set_speed(uint8_t speed) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (speed > 100) {
        speed = 100;
    }
    
    // Calculate duty cycle
    uint32_t duty = (speed * PWM_MAX_DUTY) / 100;
    
    // Invert if configured
    if (fan_config.inverted) {
        duty = PWM_MAX_DUTY - duty;
    }
    
    // Set duty cycle
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, fan_config.pwm_channel, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty cycle");
        return ret;
    }
    
    // Update duty
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, fan_config.pwm_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update duty cycle");
        return ret;
    }
    
    current_speed = speed;
    ESP_LOGD(TAG, "Fan speed set to %d%%", speed);
    
    return ESP_OK;
}

uint8_t fan_control_get_speed(void) {
    return current_speed;
}

esp_err_t fan_control_on(void) {
    return fan_control_set_speed(100);
}

esp_err_t fan_control_off(void) {
    return fan_control_set_speed(0);
}

bool fan_control_is_running(void) {
    return (current_speed > 0);
}

esp_err_t fan_control_deinit(void) {
    if (initialized) {
        ledc_stop(LEDC_LOW_SPEED_MODE, fan_config.pwm_channel, 0);
        initialized = false;
        current_speed = 0;
        ESP_LOGI(TAG, "Fan control deinitialized");
    }
    return ESP_OK;
}
