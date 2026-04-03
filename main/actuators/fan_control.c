#include "fan_control.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "FAN_CONTROL";

esp_err_t fan_control_init(fan_instance_t *inst, const fan_config_t *config) {
    if (inst == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(inst, 0, sizeof(fan_instance_t));
    memcpy(&inst->config, config, sizeof(fan_config_t));
    
    if (inst->config.pwm_frequency == 0) {
        inst->config.pwm_frequency = 5000;
    }
    if (inst->config.pwm_resolution == 0) {
        inst->config.pwm_resolution = LEDC_TIMER_10_BIT;
    }
    
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = inst->config.pwm_resolution,
        .timer_num = inst->config.pwm_timer,
        .freq_hz = inst->config.pwm_frequency,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer");
        return ret;
    }
    
    ledc_channel_config_t channel_conf = {
        .gpio_num = inst->config.gpio_num,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = inst->config.pwm_channel,
        .timer_sel = inst->config.pwm_timer,
        .duty = 0,
        .hpoint = 0,
        .intr_type = LEDC_INTR_DISABLE
    };
    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel");
        return ret;
    }
    
    inst->current_speed = 0;
    inst->initialized = true;
    
    ESP_LOGI(TAG, "Fan control initialized on GPIO %d, PWM channel %d", 
             inst->config.gpio_num, inst->config.pwm_channel);
    
    return ESP_OK;
}

esp_err_t fan_control_set_speed(fan_instance_t *inst, uint8_t speed) {
    if (inst == NULL || !inst->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (speed > 100) {
        speed = 100;
    }
    
    uint32_t duty = (speed * 1023) / 100;
    
    if (inst->config.inverted) {
        duty = 1023 - duty;
    }
    
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, inst->config.pwm_channel, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty cycle");
        return ret;
    }
    
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, inst->config.pwm_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update duty cycle");
        return ret;
    }
    
    inst->current_speed = speed;
    ESP_LOGD(TAG, "Fan speed set to %d%%", speed);
    
    return ESP_OK;
}

uint8_t fan_control_get_speed(fan_instance_t *inst) {
    if (inst == NULL) {
        return 0;
    }
    return inst->current_speed;
}

esp_err_t fan_control_on(fan_instance_t *inst) {
    return fan_control_set_speed(inst, 100);
}

esp_err_t fan_control_off(fan_instance_t *inst) {
    return fan_control_set_speed(inst, 0);
}

bool fan_control_is_running(fan_instance_t *inst) {
    if (inst == NULL) {
        return false;
    }
    return (inst->current_speed > 0);
}

esp_err_t fan_control_deinit(fan_instance_t *inst) {
    if (inst == NULL || !inst->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ledc_stop(LEDC_LOW_SPEED_MODE, inst->config.pwm_channel, 0);
    inst->initialized = false;
    inst->current_speed = 0;
    ESP_LOGI(TAG, "Fan control deinitialized");
    return ESP_OK;
}
