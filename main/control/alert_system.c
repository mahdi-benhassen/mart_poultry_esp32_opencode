#include "alert_system.h"
#include "../actuators/actuator_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "../connectivity/mqtt_client.h"
#include <string.h>
#include <stdio.h>
static const char *TAG = "ALERT_SYSTEM";

#define MAX_ALERT_HISTORY 50

static alert_config_t alert_config = {0};
static alert_type_t active_alert = ALERT_NONE;
static alert_event_t alert_history[MAX_ALERT_HISTORY] = {0};
static size_t alert_history_count = 0;
static uint32_t last_alert_time[ALERT_SYSTEM_ERROR + 1] = {0};
static bool initialized = false;

// Alert thresholds
static float temp_high_threshold = TEMP_MAX_THRESHOLD;
static float temp_low_threshold = TEMP_MIN_THRESHOLD;
static float humidity_high_threshold = HUMIDITY_MAX_THRESHOLD;
static float humidity_low_threshold = HUMIDITY_MIN_THRESHOLD;
static float ammonia_high_threshold = AMMONIA_MAX_THRESHOLD;
static float co2_high_threshold = CO2_MAX_THRESHOLD;
static float water_low_threshold = WATER_LEVEL_MIN;
static float feed_low_threshold = FEED_LEVEL_MIN;
static float gas_leak_threshold = 1000.0f;  // 1000 ppm default

esp_err_t alert_system_init(const alert_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&alert_config, config, sizeof(alert_config_t));
    
    // Initialize alert history
    memset(alert_history, 0, sizeof(alert_history));
    alert_history_count = 0;
    active_alert = ALERT_NONE;
    
    // Initialize last alert times
    memset(last_alert_time, 0, sizeof(last_alert_time));
    
    initialized = true;
    
    ESP_LOGI(TAG, "Alert system initialized: enabled=%d, mqtt=%d, alarm=%d, cooldown=%ds",
             alert_config.enabled, alert_config.mqtt_enabled, 
             alert_config.alarm_enabled, alert_config.cooldown_seconds);
    
    return ESP_OK;
}

esp_err_t alert_system_check(const sensor_data_t *sensor_data) {
    if (!initialized || !alert_config.enabled || sensor_data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint32_t current_time = esp_timer_get_time() / 1000000; // Convert to seconds
    
    // Check temperature high
    if (sensor_data->temperature > temp_high_threshold) {
        if (current_time - last_alert_time[ALERT_TEMP_HIGH] > alert_config.cooldown_seconds) {
            alert_system_trigger(ALERT_TEMP_HIGH, sensor_data->temperature, 
                               temp_high_threshold, "Temperature too high");
            last_alert_time[ALERT_TEMP_HIGH] = current_time;
        }
    }
    
    // Check temperature low
    if (sensor_data->temperature < temp_low_threshold) {
        if (current_time - last_alert_time[ALERT_TEMP_LOW] > alert_config.cooldown_seconds) {
            alert_system_trigger(ALERT_TEMP_LOW, sensor_data->temperature, 
                               temp_low_threshold, "Temperature too low");
            last_alert_time[ALERT_TEMP_LOW] = current_time;
        }
    }
    
    // Check humidity high
    if (sensor_data->humidity > humidity_high_threshold) {
        if (current_time - last_alert_time[ALERT_HUMIDITY_HIGH] > alert_config.cooldown_seconds) {
            alert_system_trigger(ALERT_HUMIDITY_HIGH, sensor_data->humidity, 
                               humidity_high_threshold, "Humidity too high");
            last_alert_time[ALERT_HUMIDITY_HIGH] = current_time;
        }
    }
    
    // Check humidity low
    if (sensor_data->humidity < humidity_low_threshold) {
        if (current_time - last_alert_time[ALERT_HUMIDITY_LOW] > alert_config.cooldown_seconds) {
            alert_system_trigger(ALERT_HUMIDITY_LOW, sensor_data->humidity, 
                               humidity_low_threshold, "Humidity too low");
            last_alert_time[ALERT_HUMIDITY_LOW] = current_time;
        }
    }
    
    // Check ammonia high
    if (sensor_data->ammonia_ppm > ammonia_high_threshold) {
        if (current_time - last_alert_time[ALERT_AMMONIA_HIGH] > alert_config.cooldown_seconds) {
            alert_system_trigger(ALERT_AMMONIA_HIGH, sensor_data->ammonia_ppm, 
                               ammonia_high_threshold, "Ammonia level critical");
            last_alert_time[ALERT_AMMONIA_HIGH] = current_time;
        }
    }
    
    // Check CO2 high
    if (sensor_data->co2_ppm > co2_high_threshold) {
        if (current_time - last_alert_time[ALERT_CO2_HIGH] > alert_config.cooldown_seconds) {
            alert_system_trigger(ALERT_CO2_HIGH, sensor_data->co2_ppm, 
                               co2_high_threshold, "CO2 level too high");
            last_alert_time[ALERT_CO2_HIGH] = current_time;
        }
    }
    
    // Check water level low
    if (sensor_data->water_level_percent < water_low_threshold) {
        if (current_time - last_alert_time[ALERT_WATER_LOW] > alert_config.cooldown_seconds) {
            alert_system_trigger(ALERT_WATER_LOW, sensor_data->water_level_percent, 
                               water_low_threshold, "Water level low");
            last_alert_time[ALERT_WATER_LOW] = current_time;
        }
    }
    
    // Check feed level low
    if (sensor_data->feed_level_percent < feed_low_threshold) {
        if (current_time - last_alert_time[ALERT_FEED_LOW] > alert_config.cooldown_seconds) {
            alert_system_trigger(ALERT_FEED_LOW, sensor_data->feed_level_percent,
                               feed_low_threshold, "Feed level low");
            last_alert_time[ALERT_FEED_LOW] = current_time;
        }
    }
    
    // Check gas leak
    if (sensor_data->gas_alarm) {
        if (current_time - last_alert_time[ALERT_GAS_LEAK] > alert_config.cooldown_seconds) {
            alert_system_trigger(ALERT_GAS_LEAK, sensor_data->gas_ppm,
                               gas_leak_threshold, "Gas leak detected!");
            last_alert_time[ALERT_GAS_LEAK] = current_time;
        }
    }
    
    return ESP_OK;
}

esp_err_t alert_system_trigger(alert_type_t type, float value, float threshold, 
                               const char *message) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Create alert event
    alert_event_t event = {
        .type = type,
        .timestamp = esp_timer_get_time() / 1000000,
        .value = value,
        .threshold = threshold
    };
    
    if (message != NULL) {
        strncpy(event.message, message, sizeof(event.message) - 1);
        event.message[sizeof(event.message) - 1] = '\0';
    }
    
    // Add to history
    if (alert_history_count < MAX_ALERT_HISTORY) {
        memcpy(&alert_history[alert_history_count], &event, sizeof(alert_event_t));
        alert_history_count++;
    } else {
        // Shift history and add new event
        memmove(&alert_history[0], &alert_history[1], 
                (MAX_ALERT_HISTORY - 1) * sizeof(alert_event_t));
        memcpy(&alert_history[MAX_ALERT_HISTORY - 1], &event, sizeof(alert_event_t));
    }
    
    // Set active alert
    active_alert = type;
    
    // Log alert
    ESP_LOGW(TAG, "ALERT: %s (Value: %.2f, Threshold: %.2f)", 
             message, value, threshold);
    
    // Trigger local alarm if enabled
    if (alert_config.alarm_enabled) {
        actuator_manager_set_alarm(true);
    }
    
    // Send MQTT alert if enabled
    if (alert_config.mqtt_enabled) {
        mqtt_client_publish_alert(active_alert, message);
    }
    
    return ESP_OK;
}

alert_type_t alert_system_get_active(void) {
    return active_alert;
}

esp_err_t alert_system_clear(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    active_alert = ALERT_NONE;
    actuator_manager_set_alarm(false);
    
    ESP_LOGI(TAG, "Alert cleared");
    return ESP_OK;
}

esp_err_t alert_system_get_history(alert_event_t *history, size_t max_count, 
                                   size_t *count) {
    if (!initialized || history == NULL || count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t copy_count = (max_count < alert_history_count) ? max_count : alert_history_count;
    memcpy(history, alert_history, copy_count * sizeof(alert_event_t));
    *count = copy_count;
    
    return ESP_OK;
}

esp_err_t alert_system_enable(bool enabled) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    alert_config.enabled = enabled;
    ESP_LOGI(TAG, "Alert system %s", enabled ? "enabled" : "disabled");
    
    return ESP_OK;
}

bool alert_system_is_enabled(void) {
    return alert_config.enabled;
}

esp_err_t alert_system_set_thresholds(float temp_high, float temp_low,
                                      float humidity_high, float humidity_low,
                                      float ammonia_high, float co2_high,
                                      float water_low, float feed_low) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    temp_high_threshold = temp_high;
    temp_low_threshold = temp_low;
    humidity_high_threshold = humidity_high;
    humidity_low_threshold = humidity_low;
    ammonia_high_threshold = ammonia_high;
    co2_high_threshold = co2_high;
    water_low_threshold = water_low;
    feed_low_threshold = feed_low;
    
    ESP_LOGI(TAG, "Alert thresholds updated");
    return ESP_OK;
}

esp_err_t alert_system_set_gas_threshold(float gas_threshold) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    gas_leak_threshold = gas_threshold;
    ESP_LOGI(TAG, "Gas leak threshold updated to %.1f ppm", gas_threshold);
    return ESP_OK;
}

esp_err_t alert_system_deinit(void) {
    initialized = false;
    active_alert = ALERT_NONE;
    alert_history_count = 0;
    ESP_LOGI(TAG, "Alert system deinitialized");
    return ESP_OK;
}
