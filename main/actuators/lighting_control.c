#include "lighting_control.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "LIGHTING_CTRL";

static lighting_config_t lighting_config = {0};
static uint8_t current_intensity = 0;
static bool initialized = false;

esp_err_t lighting_control_init(const lighting_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&lighting_config, config, sizeof(lighting_config_t));
    
    // Set defaults if not specified
    if (lighting_config.pwm_frequency == 0) {
        lighting_config.pwm_frequency = PWM_FREQUENCY;
    }
    if (lighting_config.pwm_resolution == 0) {
        lighting_config.pwm_resolution = PWM_RESOLUTION;
    }
    
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = lighting_config.pwm_resolution,
        .timer_num = lighting_config.pwm_timer,
        .freq_hz = lighting_config.pwm_frequency,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer");
        return ret;
    }
    
    // Configure LEDC channel
    ledc_channel_config_t channel_conf = {
        .gpio_num = lighting_config.gpio_num,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = lighting_config.pwm_channel,
        .timer_sel = lighting_config.pwm_timer,
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
    current_intensity = 0;
    initialized = true;
    
    ESP_LOGI(TAG, "Lighting control initialized on GPIO %d, PWM channel %d", 
             lighting_config.gpio_num, lighting_config.pwm_channel);
    
    return ESP_OK;
}

esp_err_t lighting_control_set_intensity(uint8_t intensity) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (intensity > 100) {
        intensity = 100;
    }
    
    // Calculate duty cycle
    uint32_t duty = (intensity * PWM_MAX_DUTY) / 100;
    
    // Invert if configured
    if (lighting_config.inverted) {
        duty = PWM_MAX_DUTY - duty;
    }
    
    // Set duty cycle
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, lighting_config.pwm_channel, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty cycle");
        return ret;
    }
    
    // Update duty
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, lighting_config.pwm_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update duty cycle");
        return ret;
    }
    
    current_intensity = intensity;
    ESP_LOGD(TAG, "Lighting intensity set to %d%%", intensity);
    
    return ESP_OK;
}

uint8_t lighting_control_get_intensity(void) {
    return current_intensity;
}

esp_err_t lighting_control_on(void) {
    return lighting_control_set_intensity(100);
}

esp_err_t lighting_control_off(void) {
    return lighting_control_set_intensity(0);
}

bool lighting_control_is_on(void) {
    return (current_intensity > 0);
}

esp_err_t lighting_control_sunrise(uint16_t duration_minutes) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (duration_minutes == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Starting sunrise simulation over %d minutes", duration_minutes);
    
    // Calculate step duration (100 steps over the duration)
    uint32_t step_duration_ms = (duration_minutes * 60 * 1000) / 100;
    
    // Gradually increase intensity
    for (uint8_t i = 0; i <= 100; i++) {
        esp_err_t ret = lighting_control_set_intensity(i);
        if (ret != ESP_OK) {
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(step_duration_ms));
    }
    
    ESP_LOGI(TAG, "Sunrise simulation complete");
    return ESP_OK;
}

esp_err_t lighting_control_sunset(uint16_t duration_minutes) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (duration_minutes == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Starting sunset simulation over %d minutes", duration_minutes);
    
    // Calculate step duration (100 steps over the duration)
    uint32_t step_duration_ms = (duration_minutes * 60 * 1000) / 100;
    
    // Gradually decrease intensity
    for (uint8_t i = 100; i > 0; i--) {
        esp_err_t ret = lighting_control_set_intensity(i);
        if (ret != ESP_OK) {
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(step_duration_ms));
    }
    
    // Ensure lights are off
    lighting_control_set_intensity(0);
    
    ESP_LOGI(TAG, "Sunset simulation complete");
    return ESP_OK;
}

esp_err_t lighting_control_deinit(void) {
    if (initialized) {
        ledc_stop(LEDC_LOW_SPEED_MODE, lighting_config.pwm_channel, 0);
        initialized = false;
        current_intensity = 0;
        ESP_LOGI(TAG, "Lighting control deinitialized");
    }
    return ESP_OK;
}
