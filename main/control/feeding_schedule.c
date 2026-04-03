#include "feeding_schedule.h"
#include "../actuators/feeder_control.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "FEEDING_SCHED";

static uint8_t feeding_schedule[24] = {0};
static bool schedule_enabled = true;
static bool initialized = false;
static int last_triggered_hour = -1;

esp_err_t feeding_schedule_init(void) {
    // Initialize with default feeding schedule
    // Typical poultry feeding: more during day, less at night
    memset(feeding_schedule, 0, sizeof(feeding_schedule));
    
    // Morning feeding (6 AM - 9 AM)
    feeding_schedule[6] = 50;
    feeding_schedule[7] = 60;
    feeding_schedule[8] = 50;
    
    // Midday feeding (11 AM - 1 PM)
    feeding_schedule[11] = 40;
    feeding_schedule[12] = 50;
    feeding_schedule[13] = 40;
    
    // Afternoon feeding (3 PM - 5 PM)
    feeding_schedule[15] = 40;
    feeding_schedule[16] = 50;
    feeding_schedule[17] = 40;
    
    // Evening feeding (6 PM - 7 PM)
    feeding_schedule[18] = 30;
    feeding_schedule[19] = 20;
    
    schedule_enabled = true;
    initialized = true;
    
    ESP_LOGI(TAG, "Feeding schedule initialized with default values");
    return ESP_OK;
}

esp_err_t feeding_schedule_set(const uint8_t schedule[24]) {
    if (!initialized || schedule == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    memcpy(feeding_schedule, schedule, sizeof(feeding_schedule));
    ESP_LOGI(TAG, "Feeding schedule updated");
    
    return ESP_OK;
}

esp_err_t feeding_schedule_get(uint8_t schedule[24]) {
    if (!initialized || schedule == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    memcpy(schedule, feeding_schedule, sizeof(feeding_schedule));
    return ESP_OK;
}

esp_err_t feeding_schedule_set_hour(uint8_t hour, uint8_t amount_grams) {
    if (!initialized || hour > 23) {
        return ESP_ERR_INVALID_ARG;
    }
    
    feeding_schedule[hour] = amount_grams;
    ESP_LOGI(TAG, "Feeding amount for hour %d set to %d grams", hour, amount_grams);
    
    return ESP_OK;
}

esp_err_t feeding_schedule_get_current(uint8_t *amount_grams) {
    if (!initialized || amount_grams == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint64_t now_us = esp_timer_get_time();
    uint64_t epoch_seconds = 1743638400ULL + (now_us / 1000000ULL);
    uint8_t current_hour = (epoch_seconds / 3600) % 24;
    
    *amount_grams = feeding_schedule[current_hour];
    return ESP_OK;
}

bool feeding_schedule_is_due(uint8_t current_hour, uint8_t current_minute, 
                            uint8_t *amount_grams) {
    if (!initialized || !schedule_enabled) {
        return false;
    }
    
    if (current_hour > 23 || current_minute > 59) {
        return false;
    }
    
    if (current_minute == 0 && feeding_schedule[current_hour] > 0 && last_triggered_hour != current_hour) {
        if (amount_grams != NULL) {
            *amount_grams = feeding_schedule[current_hour];
        }
        last_triggered_hour = current_hour;
        return true;
    }
    
    if (current_minute != 0) {
        last_triggered_hour = -1;
    }
    
    return false;
}

esp_err_t feeding_schedule_execute(uint8_t amount_grams) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (amount_grams == 0) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Executing feeding: %d grams", amount_grams);
    
    // Execute feeding using feeder control
    esp_err_t ret = feeder_control_dispense((float)amount_grams);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute feeding");
        return ret;
    }
    
    ESP_LOGI(TAG, "Feeding complete");
    return ESP_OK;
}

esp_err_t feeding_schedule_enable(bool enabled) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    schedule_enabled = enabled;
    ESP_LOGI(TAG, "Feeding schedule %s", enabled ? "enabled" : "disabled");
    
    return ESP_OK;
}

bool feeding_schedule_is_enabled(void) {
    return schedule_enabled;
}

esp_err_t feeding_schedule_deinit(void) {
    initialized = false;
    schedule_enabled = false;
    ESP_LOGI(TAG, "Feeding schedule deinitialized");
    return ESP_OK;
}
