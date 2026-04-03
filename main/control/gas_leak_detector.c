#include "gas_leak_detector.h"
#include "../actuators/buzzer_control.h"
#include "../actuators/actuator_manager.h"
#include "../connectivity/poultry_mqtt.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "GAS_LEAK_DETECTOR";

#define MAX_GAS_LEAK_HISTORY 50

static gas_leak_config_t detector_config = {0};
static gas_leak_severity_t current_severity = GAS_LEAK_NONE;
static gas_leak_event_t event_history[MAX_GAS_LEAK_HISTORY] = {0};
static size_t event_history_count = 0;
static bool initialized = false;

/**
 * @brief Get severity name as string
 */
static const char* get_severity_name(gas_leak_severity_t severity) {
    switch (severity) {
        case GAS_LEAK_NONE:
            return "None";
        case GAS_LEAK_LOW:
            return "Low";
        case GAS_LEAK_MODERATE:
            return "Moderate";
        case GAS_LEAK_HIGH:
            return "High";
        case GAS_LEAK_CRITICAL:
            return "Critical";
        default:
            return "Unknown";
    }
}

/**
 * @brief Get buzzer pattern for severity
 */
static buzzer_pattern_t get_buzzer_pattern(gas_leak_severity_t severity) {
    switch (severity) {
        case GAS_LEAK_LOW:
            return BUZZER_PATTERN_INTERMITTENT;
        case GAS_LEAK_MODERATE:
            return BUZZER_PATTERN_PULSE;
        case GAS_LEAK_HIGH:
            return BUZZER_PATTERN_SIREN;
        case GAS_LEAK_CRITICAL:
            return BUZZER_PATTERN_EMERGENCY;
        default:
            return BUZZER_PATTERN_CONTINUOUS;
    }
}

/**
 * @brief Get buzzer duration for severity
 */
static uint32_t get_buzzer_duration(gas_leak_severity_t severity) {
    switch (severity) {
        case GAS_LEAK_LOW:
            return 5000;  // 5 seconds
        case GAS_LEAK_MODERATE:
            return 10000; // 10 seconds
        case GAS_LEAK_HIGH:
            return 30000; // 30 seconds
        case GAS_LEAK_CRITICAL:
            return 0;     // Infinite (until manually stopped)
        default:
            return 5000;
    }
}

/**
 * @brief Add event to history
 */
static void add_event_to_history(const gas_leak_event_t *event) {
    if (event_history_count < MAX_GAS_LEAK_HISTORY) {
        memcpy(&event_history[event_history_count], event, sizeof(gas_leak_event_t));
        event_history_count++;
    } else {
        // Shift history and add new event
        memmove(&event_history[0], &event_history[1], 
                (MAX_GAS_LEAK_HISTORY - 1) * sizeof(gas_leak_event_t));
        memcpy(&event_history[MAX_GAS_LEAK_HISTORY - 1], event, sizeof(gas_leak_event_t));
    }
}

esp_err_t gas_leak_detector_init(const gas_leak_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&detector_config, config, sizeof(gas_leak_config_t));
    
    // Set defaults if not specified
    if (detector_config.low_threshold_ppm == 0) {
        detector_config.low_threshold_ppm = 1000.0f;  // 1000 ppm
    }
    if (detector_config.moderate_threshold_ppm == 0) {
        detector_config.moderate_threshold_ppm = 2500.0f;  // 2500 ppm
    }
    if (detector_config.high_threshold_ppm == 0) {
        detector_config.high_threshold_ppm = 5000.0f;  // 5000 ppm
    }
    if (detector_config.critical_threshold_ppm == 0) {
        detector_config.critical_threshold_ppm = 10000.0f;  // 10000 ppm
    }
    if (detector_config.detection_interval_ms == 0) {
        detector_config.detection_interval_ms = 1000;  // 1 second
    }
    
    // Initialize event history
    memset(event_history, 0, sizeof(event_history));
    event_history_count = 0;
    current_severity = GAS_LEAK_NONE;
    
    initialized = true;
    
    ESP_LOGI(TAG, "Gas leak detector initialized");
    ESP_LOGI(TAG, "Thresholds: Low=%.1f, Moderate=%.1f, High=%.1f, Critical=%.1f ppm",
             detector_config.low_threshold_ppm,
             detector_config.moderate_threshold_ppm,
             detector_config.high_threshold_ppm,
             detector_config.critical_threshold_ppm);
    
    return ESP_OK;
}

esp_err_t gas_leak_detector_check(gas_leak_event_t *event) {
    if (!initialized || !detector_config.enabled) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Read gas sensor
    gas_sensor_data_t gas_data;
    esp_err_t ret = gas_sensor_read(&gas_data);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read gas sensor");
        return ret;
    }
    
    // Determine severity based on concentration
    gas_leak_severity_t new_severity = GAS_LEAK_NONE;
    
    if (gas_data.concentration_ppm >= detector_config.critical_threshold_ppm) {
        new_severity = GAS_LEAK_CRITICAL;
    } else if (gas_data.concentration_ppm >= detector_config.high_threshold_ppm) {
        new_severity = GAS_LEAK_HIGH;
    } else if (gas_data.concentration_ppm >= detector_config.moderate_threshold_ppm) {
        new_severity = GAS_LEAK_MODERATE;
    } else if (gas_data.concentration_ppm >= detector_config.low_threshold_ppm) {
        new_severity = GAS_LEAK_LOW;
    }
    
    // Check if severity changed
    if (new_severity != current_severity) {
        current_severity = new_severity;
        
        // Create event
        event->severity = new_severity;
        event->gas_type = gas_data.gas_type;
        event->concentration_ppm = gas_data.concentration_ppm;
        event->percentage_lel = gas_data.percentage;
        event->timestamp = esp_timer_get_time() / 1000000;
        
        // Create message
        snprintf(event->message, sizeof(event->message),
                 "Gas leak detected: %s level (%.1f ppm, %.1f%% LEL)",
                 get_severity_name(new_severity),
                 gas_data.concentration_ppm,
                 gas_data.percentage);
        
        // Add to history
        add_event_to_history(event);
        
        // Log event
        if (new_severity == GAS_LEAK_CRITICAL) {
            ESP_LOGE(TAG, "CRITICAL GAS LEAK! %s", event->message);
        } else if (new_severity == GAS_LEAK_HIGH) {
            ESP_LOGE(TAG, "HIGH GAS LEAK! %s", event->message);
        } else if (new_severity == GAS_LEAK_MODERATE) {
            ESP_LOGW(TAG, "MODERATE GAS LEAK: %s", event->message);
        } else if (new_severity == GAS_LEAK_LOW) {
            ESP_LOGW(TAG, "LOW GAS LEAK: %s", event->message);
        } else {
            ESP_LOGI(TAG, "Gas leak cleared");
        }
        
        // Trigger alerts based on severity
        if (new_severity != GAS_LEAK_NONE) {
            // Trigger buzzer alert
            if (detector_config.buzzer_enabled) {
                buzzer_pattern_t pattern = get_buzzer_pattern(new_severity);
                uint32_t duration = get_buzzer_duration(new_severity);
                buzzer_alarm_start(pattern, duration, 100);
            }
            
            // Trigger MQTT notification
            if (detector_config.mqtt_enabled) {
                mqtt_client_publish_alert(ALERT_GAS_LEAK, event->message);
            }
            
            // Enable automatic ventilation for high/critical leaks
            if (detector_config.auto_ventilation && 
                (new_severity == GAS_LEAK_HIGH || new_severity == GAS_LEAK_CRITICAL)) {
                ESP_LOGW(TAG, "Enabling emergency ventilation");
                actuator_manager_set_ventilation(100);  // Open ventilation fully
                actuator_manager_set_exhaust_fan(100);   // Turn on exhaust fan
            }
        } else {
            // Gas leak cleared - stop buzzer
            if (detector_config.buzzer_enabled) {
                buzzer_alarm_stop();
            }
        }
    }
    
    return ESP_OK;
}

gas_leak_severity_t gas_leak_detector_get_severity(void) {
    return current_severity;
}

esp_err_t gas_leak_detector_get_history(gas_leak_event_t *events, size_t max_count, 
                                        size_t *count) {
    if (!initialized || events == NULL || count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t copy_count = (max_count < event_history_count) ? max_count : event_history_count;
    memcpy(events, event_history, copy_count * sizeof(gas_leak_event_t));
    *count = copy_count;
    
    return ESP_OK;
}

esp_err_t gas_leak_detector_clear_history(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    event_history_count = 0;
    memset(event_history, 0, sizeof(event_history));
    
    ESP_LOGI(TAG, "Gas leak event history cleared");
    return ESP_OK;
}

esp_err_t gas_leak_detector_set_thresholds(float low, float moderate, 
                                           float high, float critical) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (low < 0 || moderate < 0 || high < 0 || critical < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    detector_config.low_threshold_ppm = low;
    detector_config.moderate_threshold_ppm = moderate;
    detector_config.high_threshold_ppm = high;
    detector_config.critical_threshold_ppm = critical;
    
    ESP_LOGI(TAG, "Gas leak thresholds updated: Low=%.1f, Moderate=%.1f, High=%.1f, Critical=%.1f ppm",
             low, moderate, high, critical);
    
    return ESP_OK;
}

esp_err_t gas_leak_detector_enable(bool enabled) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    detector_config.enabled = enabled;
    ESP_LOGI(TAG, "Gas leak detection %s", enabled ? "enabled" : "disabled");
    
    return ESP_OK;
}

bool gas_leak_detector_is_enabled(void) {
    return detector_config.enabled;
}

esp_err_t gas_leak_detector_trigger_alert(gas_leak_severity_t severity, 
                                          const char *message) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Create event
    gas_leak_event_t event = {
        .severity = severity,
        .gas_type = GAS_TYPE_GENERAL,
        .concentration_ppm = 0,
        .percentage_lel = 0,
        .timestamp = esp_timer_get_time() / 1000000
    };
    
    if (message != NULL) {
        strncpy(event.message, message, sizeof(event.message) - 1);
        event.message[sizeof(event.message) - 1] = '\0';
    } else {
        snprintf(event.message, sizeof(event.message),
                 "Manual gas leak alert: %s level", get_severity_name(severity));
    }
    
    // Add to history
    add_event_to_history(&event);
    
    // Update current severity
    current_severity = severity;
    
    // Log event
    ESP_LOGW(TAG, "Manual gas leak alert triggered: %s", event.message);
    
    // Trigger buzzer alert
    if (detector_config.buzzer_enabled) {
        buzzer_pattern_t pattern = get_buzzer_pattern(severity);
        uint32_t duration = get_buzzer_duration(severity);
        buzzer_alarm_start(pattern, duration, 100);
    }
    
    // Trigger MQTT notification
    if (detector_config.mqtt_enabled) {
        mqtt_client_publish_alert(ALERT_GAS_LEAK, event.message);
    }
    
    return ESP_OK;
}

esp_err_t gas_leak_detector_deinit(void) {
    if (initialized) {
        // Stop buzzer if active
        if (detector_config.buzzer_enabled) {
            buzzer_alarm_stop();
        }
        
        initialized = false;
        current_severity = GAS_LEAK_NONE;
        event_history_count = 0;
        
        ESP_LOGI(TAG, "Gas leak detector deinitialized");
    }
    return ESP_OK;
}
