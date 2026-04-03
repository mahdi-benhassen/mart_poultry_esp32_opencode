#include "heater_control.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "HEATER_CONTROL";

static heater_config_t heater_config = {0};
static uint8_t current_power = 0;
static bool initialized = false;

esp_err_t heater_control_init(const heater_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&heater_config, config, sizeof(heater_config_t));
    
    // Set defaults if not specified
    if (heater_config.pwm_frequency == 0) {
        heater_config.pwm_frequency = PWM_FREQUENCY;
    }
    if (heater_config.pwm_resolution == 0) {
        heater_config.pwm_resolution = PWM_RESOLUTION;
    }
    if (heater_config.max_temperature == 0) {
        heater_config.max_temperature = 40.0f; // Default 40°C max
    }
    
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = heater_config.pwm_resolution,
        .timer_num = heater_config.pwm_timer,
        .freq_hz = heater_config.pwm_frequency,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer");
        return ret;
    }
    
    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = heater_config.gpio_num,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = heater_config.pwm_channel,
        .timer_sel = heater_config.pwm_timer,
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
    current_power = 0;
    initialized = true;
    
    ESP_LOGI(TAG, "Heater control initialized on GPIO %d, PWM channel %d", 
             heater_config.gpio_num, heater_config.pwm_channel);
    
    return ESP_OK;
}

esp_err_t heater_control_set_power(uint8_t power) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (power > 100) {
        power = 100;
    }
    
    // Calculate duty cycle
    uint32_t max_duty = (1 << 10) - 1;
    uint32_t duty = (power * max_duty) / 100;
    
    if (heater_config.inverted) {
        duty = max_duty - duty;
    }
    
    // Set duty cycle
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, heater_config.pwm_channel, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty cycle");
        return ret;
    }
    
    // Update duty
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, heater_config.pwm_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update duty cycle");
        return ret;
    }
    
    current_power = power;
    ESP_LOGD(TAG, "Heater power set to %d%%", power);
    
    return ESP_OK;
}

uint8_t heater_control_get_power(void) {
    return current_power;
}

esp_err_t heater_control_on(void) {
    return heater_control_set_power(100);
}

esp_err_t heater_control_off(void) {
    return heater_control_set_power(0);
}

bool heater_control_is_on(void) {
    return (current_power > 0);
}

esp_err_t heater_control_set_limit(float max_temp) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (max_temp < 0 || max_temp > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    
    heater_config.max_temperature = max_temp;
    ESP_LOGI(TAG, "Heater temperature limit set to %.1f°C", max_temp);
    
    return ESP_OK;
}

esp_err_t heater_control_deinit(void) {
    if (initialized) {
        ledc_stop(LEDC_LOW_SPEED_MODE, heater_config.pwm_channel, 0);
        initialized = false;
        current_power = 0;
        ESP_LOGI(TAG, "Heater control deinitialized");
    }
    return ESP_OK;
}
