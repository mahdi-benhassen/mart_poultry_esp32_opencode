#include "feeder_control.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../include/poultry_system_config.h"
#include <string.h>

static const char *TAG = "FEEDER_CONTROL";

static feeder_config_t feeder_config = {0};
static uint8_t current_speed = 0;
static bool is_running = false;
static bool initialized = false;

esp_err_t feeder_control_init(const feeder_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&feeder_config, config, sizeof(feeder_config_t));
    
    // Set defaults if not specified
    if (feeder_config.pwm_frequency == 0) {
        feeder_config.pwm_frequency = PWM_FREQUENCY;
    }
    if (feeder_config.pwm_resolution == 0) {
        feeder_config.pwm_resolution = PWM_RESOLUTION;
    }
    if (feeder_config.feed_rate_g_per_sec == 0) {
        feeder_config.feed_rate_g_per_sec = 10.0f; // Default 10g/sec
    }
    
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = feeder_config.pwm_resolution,
        .timer_num = feeder_config.pwm_timer,
        .freq_hz = feeder_config.pwm_frequency,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer");
        return ret;
    }
    
    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = feeder_config.gpio_num,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = feeder_config.pwm_channel,
        .timer_sel = feeder_config.pwm_timer,
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
    is_running = false;
    initialized = true;
    
    ESP_LOGI(TAG, "Feeder control initialized on GPIO %d, PWM channel %d", 
             feeder_config.gpio_num, feeder_config.pwm_channel);
    
    return ESP_OK;
}

esp_err_t feeder_control_set_speed(uint8_t speed) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (speed > 100) {
        speed = 100;
    }
    
    // Calculate duty cycle
    uint32_t duty = (speed * PWM_MAX_DUTY) / 100;
    
    // Invert if configured
    if (feeder_config.inverted) {
        duty = PWM_MAX_DUTY - duty;
    }
    
    // Set duty cycle
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, feeder_config.pwm_channel, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty cycle");
        return ret;
    }
    
    // Update duty
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, feeder_config.pwm_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update duty cycle");
        return ret;
    }
    
    current_speed = speed;
    is_running = (speed > 0);
    ESP_LOGD(TAG, "Feeder speed set to %d%%", speed);
    
    return ESP_OK;
}

uint8_t feeder_control_get_speed(void) {
    return current_speed;
}

esp_err_t feeder_control_dispense(float grams) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (grams <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Calculate time needed to dispense the specified amount
    float seconds = grams / feeder_config.feed_rate_g_per_sec;
    uint32_t milliseconds = (uint32_t)(seconds * 1000);
    
    ESP_LOGI(TAG, "Dispensing %.1f grams over %.1f seconds", grams, seconds);
    
    // Start feeder
    esp_err_t ret = feeder_control_set_speed(100);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Wait for specified time
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
    
    // Stop feeder
    ret = feeder_control_set_speed(0);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ESP_LOGI(TAG, "Dispensing complete");
    return ESP_OK;
}

esp_err_t feeder_control_start(void) {
    return feeder_control_set_speed(100);
}

esp_err_t feeder_control_stop(void) {
    return feeder_control_set_speed(0);
}

bool feeder_control_is_running(void) {
    return is_running;
}

esp_err_t feeder_control_set_rate(float rate) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (rate <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    feeder_config.feed_rate_g_per_sec = rate;
    ESP_LOGI(TAG, "Feed rate set to %.1f g/sec", rate);
    
    return ESP_OK;
}

esp_err_t feeder_control_deinit(void) {
    if (initialized) {
        ledc_stop(LEDC_LOW_SPEED_MODE, feeder_config.pwm_channel, 0);
        initialized = false;
        current_speed = 0;
        is_running = false;
        ESP_LOGI(TAG, "Feeder control deinitialized");
    }
    return ESP_OK;
}
