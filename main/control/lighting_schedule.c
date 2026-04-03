#include "lighting_schedule.h"
#include "../actuators/lighting_control.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "LIGHTING_SCHED";

static uint8_t lighting_schedule[24] = {0};
static bool schedule_enabled = true;
static bool initialized = false;

esp_err_t lighting_schedule_init(void) {
    // Initialize with default lighting schedule
    // Typical poultry lighting: 16-18 hours of light, 6-8 hours of dark
    memset(lighting_schedule, 0, sizeof(lighting_schedule));
    
    // Night (0-5 AM): 0% intensity
    lighting_schedule[0] = 0;
    lighting_schedule[1] = 0;
    lighting_schedule[2] = 0;
    lighting_schedule[3] = 0;
    lighting_schedule[4] = 0;
    lighting_schedule[5] = 0;
    
    // Sunrise (6 AM): Gradually increase
    lighting_schedule[6] = 20;
    lighting_schedule[7] = 40;
    lighting_schedule[8] = 60;
    
    // Day (9 AM - 5 PM): Full intensity
    for (int i = 9; i <= 17; i++) {
        lighting_schedule[i] = 100;
    }
    
    // Sunset (6 PM - 8 PM): Gradually decrease
    lighting_schedule[18] = 80;
    lighting_schedule[19] = 60;
    lighting_schedule[20] = 40;
    
    // Night (9 PM - 11 PM): Low intensity
    lighting_schedule[21] = 20;
    lighting_schedule[22] = 10;
    lighting_schedule[23] = 0;
    
    schedule_enabled = true;
    initialized = true;
    
    ESP_LOGI(TAG, "Lighting schedule initialized with default values");
    return ESP_OK;
}

esp_err_t lighting_schedule_set(const uint8_t schedule[24]) {
    if (!initialized || schedule == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    memcpy(lighting_schedule, schedule, sizeof(lighting_schedule));
    ESP_LOGI(TAG, "Lighting schedule updated");
    
    return ESP_OK;
}

esp_err_t lighting_schedule_get(uint8_t schedule[24]) {
    if (!initialized || schedule == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    memcpy(schedule, lighting_schedule, sizeof(lighting_schedule));
    return ESP_OK;
}

esp_err_t lighting_schedule_set_hour(uint8_t hour, uint8_t intensity) {
    if (!initialized || hour > 23) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (intensity > 100) {
        intensity = 100;
    }
    
    lighting_schedule[hour] = intensity;
    ESP_LOGI(TAG, "Lighting intensity for hour %d set to %d%%", hour, intensity);
    
    return ESP_OK;
}

esp_err_t lighting_schedule_get_current(uint8_t *intensity) {
    if (!initialized || intensity == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint64_t now_us = esp_timer_get_time();
    uint64_t epoch_seconds = 1743638400ULL + (now_us / 1000000ULL);
    uint8_t current_hour = (epoch_seconds / 3600) % 24;
    
    *intensity = lighting_schedule[current_hour];
    return ESP_OK;
}

bool lighting_schedule_is_due(uint8_t current_hour, uint8_t current_minute, 
                             uint8_t *intensity) {
    if (!initialized || !schedule_enabled) {
        return false;
    }
    
    if (current_hour > 23 || current_minute > 59) {
        return false;
    }
    
    // Check if lighting change is due (at the start of each hour)
    if (current_minute == 0) {
        if (intensity != NULL) {
            *intensity = lighting_schedule[current_hour];
        }
        return true;
    }
    
    return false;
}

esp_err_t lighting_schedule_execute(uint8_t intensity) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (intensity > 100) {
        intensity = 100;
    }
    
    ESP_LOGI(TAG, "Executing lighting change: %d%%", intensity);
    
    // Execute lighting change using lighting control
    esp_err_t ret = lighting_control_set_intensity(intensity);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute lighting change");
        return ret;
    }
    
    ESP_LOGI(TAG, "Lighting change complete");
    return ESP_OK;
}

esp_err_t lighting_schedule_enable(bool enabled) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    schedule_enabled = enabled;
    ESP_LOGI(TAG, "Lighting schedule %s", enabled ? "enabled" : "disabled");
    
    return ESP_OK;
}

bool lighting_schedule_is_enabled(void) {
    return schedule_enabled;
}

esp_err_t lighting_schedule_set_daynight(uint8_t day_start_hour, uint8_t day_end_hour,
                                         uint8_t day_intensity, uint8_t night_intensity) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (day_start_hour > 23 || day_end_hour > 23) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (day_intensity > 100 || night_intensity > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Set all hours to night intensity first
    for (int i = 0; i < 24; i++) {
        lighting_schedule[i] = night_intensity;
    }
    
    // Set day hours to day intensity
    if (day_start_hour <= day_end_hour) {
        for (int i = day_start_hour; i <= day_end_hour; i++) {
            lighting_schedule[i] = day_intensity;
        }
    } else {
        // Handle overnight day period
        for (int i = day_start_hour; i < 24; i++) {
            lighting_schedule[i] = day_intensity;
        }
        for (int i = 0; i <= day_end_hour; i++) {
            lighting_schedule[i] = day_intensity;
        }
    }
    
    ESP_LOGI(TAG, "Day/night cycle set: Day %d:00-%d:00 at %d%%, Night at %d%%",
             day_start_hour, day_end_hour, day_intensity, night_intensity);
    
    return ESP_OK;
}

esp_err_t lighting_schedule_deinit(void) {
    initialized = false;
    schedule_enabled = false;
    ESP_LOGI(TAG, "Lighting schedule deinitialized");
    return ESP_OK;
}
