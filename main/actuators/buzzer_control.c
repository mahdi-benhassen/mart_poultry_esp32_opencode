#include "buzzer_control.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BUZZER_CONTROL";

static buzzer_config_t buzzer_config = {0};
static buzzer_state_t buzzer_state = {0};
static bool initialized = false;
static TaskHandle_t buzzer_task_handle = NULL;

/**
 * @brief Buzzer pattern task
 */
static void buzzer_pattern_task(void *pvParameters) {
    ESP_LOGI(TAG, "Buzzer pattern task started");
    
    while (buzzer_state.is_active) {
        // Check if duration has expired
        if (buzzer_state.duration_ms > 0) {
            uint32_t elapsed = (esp_timer_get_time() / 1000) - buzzer_state.start_time;
            if (elapsed >= buzzer_state.duration_ms) {
                ESP_LOGI(TAG, "Buzzer alarm duration expired");
                buzzer_alarm_stop();
                break;
            }
        }
        
        // Execute pattern
        switch (buzzer_state.pattern) {
            case BUZZER_PATTERN_CONTINUOUS:
                // Continuous - just keep GPIO on
                gpio_set_level(buzzer_config.gpio_num, 
                              buzzer_config.active_high ? 1 : 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
                
            case BUZZER_PATTERN_PULSE:
                // Pulsing - 500ms on, 500ms off
                gpio_set_level(buzzer_config.gpio_num, 
                              buzzer_config.active_high ? 1 : 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(buzzer_config.gpio_num, 
                              buzzer_config.active_high ? 0 : 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
                
            case BUZZER_PATTERN_SIREN:
                // Siren - rising and falling tone (simulated with on/off)
                for (int i = 0; i < 10 && buzzer_state.is_active; i++) {
                    gpio_set_level(buzzer_config.gpio_num, 
                                  buzzer_config.active_high ? 1 : 0);
                    vTaskDelay(pdMS_TO_TICKS(50 + i * 20));
                    gpio_set_level(buzzer_config.gpio_num, 
                                  buzzer_config.active_high ? 0 : 1);
                    vTaskDelay(pdMS_TO_TICKS(50 + i * 20));
                }
                for (int i = 10; i > 0 && buzzer_state.is_active; i--) {
                    gpio_set_level(buzzer_config.gpio_num, 
                                  buzzer_config.active_high ? 1 : 0);
                    vTaskDelay(pdMS_TO_TICKS(50 + i * 20));
                    gpio_set_level(buzzer_config.gpio_num, 
                                  buzzer_config.active_high ? 0 : 1);
                    vTaskDelay(pdMS_TO_TICKS(50 + i * 20));
                }
                break;
                
            case BUZZER_PATTERN_INTERMITTENT:
                // Intermittent - 200ms on, 800ms off
                gpio_set_level(buzzer_config.gpio_num, 
                              buzzer_config.active_high ? 1 : 0);
                vTaskDelay(pdMS_TO_TICKS(200));
                gpio_set_level(buzzer_config.gpio_num, 
                              buzzer_config.active_high ? 0 : 1);
                vTaskDelay(pdMS_TO_TICKS(800));
                break;
                
            case BUZZER_PATTERN_EMERGENCY:
                // Emergency - rapid beeps (100ms on, 100ms off)
                gpio_set_level(buzzer_config.gpio_num, 
                              buzzer_config.active_high ? 1 : 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_set_level(buzzer_config.gpio_num, 
                              buzzer_config.active_high ? 0 : 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
    
    // Ensure buzzer is off when task exits
    gpio_set_level(buzzer_config.gpio_num, 
                  buzzer_config.active_high ? 0 : 1);
    
    ESP_LOGI(TAG, "Buzzer pattern task ended");
    buzzer_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t buzzer_control_init(const buzzer_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&buzzer_config, config, sizeof(buzzer_config_t));
    
    // Set defaults if not specified
    if (buzzer_config.frequency_hz == 0) {
        buzzer_config.frequency_hz = 2000; // 2kHz default
    }
    if (buzzer_config.duty_cycle == 0) {
        buzzer_config.duty_cycle = 50; // 50% duty cycle
    }
    
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << buzzer_config.gpio_num),
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
    gpio_set_level(buzzer_config.gpio_num, 
                  buzzer_config.active_high ? 0 : 1);
    
    // Initialize state
    memset(&buzzer_state, 0, sizeof(buzzer_state_t));
    buzzer_state.is_active = false;
    
    initialized = true;
    ESP_LOGI(TAG, "Buzzer control initialized on GPIO %d", buzzer_config.gpio_num);
    
    return ESP_OK;
}

esp_err_t buzzer_alarm_start(buzzer_pattern_t pattern, uint32_t duration_ms, uint8_t intensity) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Stop any existing alarm
    if (buzzer_state.is_active) {
        buzzer_alarm_stop();
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait for task to stop
    }
    
    // Set alarm state
    buzzer_state.is_active = true;
    buzzer_state.pattern = pattern;
    buzzer_state.start_time = esp_timer_get_time() / 1000;
    buzzer_state.duration_ms = duration_ms;
    buzzer_state.intensity = intensity > 100 ? 100 : intensity;
    
    ESP_LOGW(TAG, "Buzzer alarm started: pattern=%d, duration=%dms, intensity=%d%%",
             pattern, duration_ms, buzzer_state.intensity);
    
    // Start pattern task
    BaseType_t ret = xTaskCreate(
        buzzer_pattern_task,
        "buzzer_task",
        2048,
        NULL,
        5,
        &buzzer_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create buzzer task");
        buzzer_state.is_active = false;
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t buzzer_alarm_stop(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!buzzer_state.is_active) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping buzzer alarm");
    
    // Set flag to stop task
    buzzer_state.is_active = false;
    
    // Wait for task to finish
    if (buzzer_task_handle != NULL) {
        // Give task time to exit
        vTaskDelay(pdMS_TO_TICKS(200));
        
        // Force delete if still running
        if (buzzer_task_handle != NULL) {
            vTaskDelete(buzzer_task_handle);
            buzzer_task_handle = NULL;
        }
    }
    
    // Ensure buzzer is off
    gpio_set_level(buzzer_config.gpio_num, 
                  buzzer_config.active_high ? 0 : 1);
    
    ESP_LOGI(TAG, "Buzzer alarm stopped");
    return ESP_OK;
}

bool buzzer_alarm_is_active(void) {
    return buzzer_state.is_active;
}

esp_err_t buzzer_alarm_get_state(buzzer_state_t *state) {
    if (!initialized || state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(state, &buzzer_state, sizeof(buzzer_state_t));
    return ESP_OK;
}

esp_err_t buzzer_alarm_set_pattern(buzzer_pattern_t pattern) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    buzzer_state.pattern = pattern;
    ESP_LOGI(TAG, "Buzzer pattern set to %d", pattern);
    
    return ESP_OK;
}

esp_err_t buzzer_alarm_set_intensity(uint8_t intensity) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (intensity > 100) {
        intensity = 100;
    }
    
    buzzer_state.intensity = intensity;
    ESP_LOGI(TAG, "Buzzer intensity set to %d%%", intensity);
    
    return ESP_OK;
}

esp_err_t buzzer_test_beep(uint32_t duration_ms) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Test beep for %dms", duration_ms);
    
    gpio_set_level(buzzer_config.gpio_num, 
                  buzzer_config.active_high ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    gpio_set_level(buzzer_config.gpio_num, 
                  buzzer_config.active_high ? 0 : 1);
    
    return ESP_OK;
}

esp_err_t buzzer_control_deinit(void) {
    if (initialized) {
        // Stop any active alarm
        if (buzzer_state.is_active) {
            buzzer_alarm_stop();
        }
        
        // Ensure buzzer is off
        gpio_set_level(buzzer_config.gpio_num, 
                      buzzer_config.active_high ? 0 : 1);
        
        initialized = false;
        ESP_LOGI(TAG, "Buzzer control deinitialized");
    }
    return ESP_OK;
}
