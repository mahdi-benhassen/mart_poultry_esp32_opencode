#include "water_pump.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "WATER_PUMP";

static water_pump_config_t pump_config = {0};
static bool is_running = false;
static bool initialized = false;

esp_err_t water_pump_init(const water_pump_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&pump_config, config, sizeof(water_pump_config_t));
    
    // Set defaults if not specified
    if (pump_config.flow_rate_ml_per_sec == 0) {
        pump_config.flow_rate_ml_per_sec = 100.0f; // Default 100ml/sec
    }
    
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pump_config.gpio_num),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO");
        return ret;
    }
    
    // Initialize to off
    gpio_set_level(pump_config.gpio_num, !pump_config.active_high);
    is_running = false;
    initialized = true;
    
    ESP_LOGI(TAG, "Water pump initialized on GPIO %d", pump_config.gpio_num);
    
    return ESP_OK;
}

esp_err_t water_pump_on(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    gpio_set_level(pump_config.gpio_num, pump_config.active_high);
    is_running = true;
    ESP_LOGD(TAG, "Water pump turned on");
    
    return ESP_OK;
}

esp_err_t water_pump_off(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    gpio_set_level(pump_config.gpio_num, !pump_config.active_high);
    is_running = false;
    ESP_LOGD(TAG, "Water pump turned off");
    
    return ESP_OK;
}

bool water_pump_is_running(void) {
    return is_running;
}

esp_err_t water_pump_dispense(float ml) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (ml <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Calculate time needed to dispense the specified amount
    float seconds = ml / pump_config.flow_rate_ml_per_sec;
    uint32_t milliseconds = (uint32_t)(seconds * 1000);
    
    ESP_LOGI(TAG, "Dispensing %.1f ml over %.1f seconds", ml, seconds);
    
    // Start pump
    esp_err_t ret = water_pump_on();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Wait for specified time
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
    
    // Stop pump
    ret = water_pump_off();
    if (ret != ESP_OK) {
        return ret;
    }
    
    ESP_LOGI(TAG, "Dispensing complete");
    return ESP_OK;
}

esp_err_t water_pump_set_flow_rate(float rate) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (rate <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    pump_config.flow_rate_ml_per_sec = rate;
    ESP_LOGI(TAG, "Flow rate set to %.1f ml/sec", rate);
    
    return ESP_OK;
}

esp_err_t water_pump_deinit(void) {
    if (initialized) {
        water_pump_off();
        initialized = false;
        ESP_LOGI(TAG, "Water pump deinitialized");
    }
    return ESP_OK;
}
