#include "data_logger.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "DATA_LOGGER";

#define MAX_LOG_ENTRIES 100
#define NVS_LOG_NAMESPACE "data_log"

static data_logger_config_t logger_config = {0};
static log_entry_t log_entries[MAX_LOG_ENTRIES] = {0};
static size_t log_count = 0;
static size_t write_index = 0;
static bool initialized = false;
static nvs_handle_t nvs_handle;

esp_err_t data_logger_init(const data_logger_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&logger_config, config, sizeof(data_logger_config_t));
    
    // Set defaults
    if (logger_config.log_interval_ms == 0) {
        logger_config.log_interval_ms = DATA_LOG_INTERVAL_MS;
    }
    if (logger_config.max_file_size == 0) {
        logger_config.max_file_size = LOG_FILE_MAX_SIZE;
    }
    if (logger_config.max_files == 0) {
        logger_config.max_files = MAX_LOG_FILES;
    }
    
    // NVS is already initialized by app_main, just open our namespace
    esp_err_t ret = nvs_open(NVS_LOG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return ret;
    }
    
    // Load existing log entries from NVS
    size_t required_size = sizeof(log_entries);
    ret = nvs_get_blob(nvs_handle, "logs", log_entries, &required_size);
    if (ret == ESP_OK) {
        log_count = required_size / sizeof(log_entry_t);
        ESP_LOGI(TAG, "Loaded %d log entries from NVS", log_count);
    } else {
        log_count = 0;
        ESP_LOGI(TAG, "No existing log entries found");
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Data logger initialized: enabled=%d, interval=%dms", 
             logger_config.enabled, logger_config.log_interval_ms);
    
    return ESP_OK;
}

esp_err_t data_logger_log(const sensor_data_t *sensor_data, 
                          const actuator_states_t *actuator_states,
                          alert_type_t active_alert) {
    if (!initialized || !logger_config.enabled) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (sensor_data == NULL || actuator_states == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Create log entry
    log_entry_t entry = {
        .timestamp = esp_timer_get_time() / 1000000,
        .active_alert = active_alert
    };
    memcpy(&entry.sensor_data, sensor_data, sizeof(sensor_data_t));
    memcpy(&entry.actuator_states, actuator_states, sizeof(actuator_states_t));
    
    // Add to log using ring buffer
    memcpy(&log_entries[write_index], &entry, sizeof(log_entry_t));
    write_index = (write_index + 1) % MAX_LOG_ENTRIES;
    if (log_count < MAX_LOG_ENTRIES) {
        log_count++;
    }
    
    // Save to NVS periodically
    static uint32_t last_save = 0;
    uint32_t current_time = esp_timer_get_time() / 1000000;
    if (current_time - last_save >= 60) {
        size_t save_size = log_count * sizeof(log_entry_t);
        if (save_size <= 0x3000) {
            nvs_set_blob(nvs_handle, "logs", log_entries, save_size);
            nvs_commit(nvs_handle);
        }
        last_save = current_time;
    }
    
    ESP_LOGD(TAG, "Logged entry: T=%.1f°C, H=%.1f%%, NH3=%.1fppm", 
             sensor_data->temperature, sensor_data->humidity, sensor_data->ammonia_ppm);
    
    return ESP_OK;
}

esp_err_t data_logger_get_entries(log_entry_t *entries, size_t max_count, 
                                  size_t *count) {
    if (!initialized || entries == NULL || count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t copy_count = (max_count < log_count) ? max_count : log_count;
    size_t start_idx = (write_index >= copy_count) ? (write_index - copy_count) : (MAX_LOG_ENTRIES - (copy_count - write_index));
    
    for (size_t i = 0; i < copy_count; i++) {
        memcpy(&entries[i], &log_entries[(start_idx + i) % MAX_LOG_ENTRIES], sizeof(log_entry_t));
    }
    *count = copy_count;
    
    return ESP_OK;
}

esp_err_t data_logger_get_entries_range(uint32_t start_time, uint32_t end_time,
                                        log_entry_t *entries, size_t max_count, 
                                        size_t *count) {
    if (!initialized || entries == NULL || count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t found = 0;
    for (size_t i = 0; i < log_count && found < max_count; i++) {
        if (log_entries[i].timestamp >= start_time && 
            log_entries[i].timestamp <= end_time) {
            memcpy(&entries[found], &log_entries[i], sizeof(log_entry_t));
            found++;
        }
    }
    
    *count = found;
    return ESP_OK;
}

esp_err_t data_logger_clear(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    log_count = 0;
    memset(log_entries, 0, sizeof(log_entries));
    
    // Clear NVS
    nvs_erase_key(nvs_handle, "logs");
    nvs_commit(nvs_handle);
    
    ESP_LOGI(TAG, "Log entries cleared");
    return ESP_OK;
}

size_t data_logger_get_count(void) {
    return log_count;
}

esp_err_t data_logger_enable(bool enabled) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    logger_config.enabled = enabled;
    ESP_LOGI(TAG, "Data logger %s", enabled ? "enabled" : "disabled");
    
    return ESP_OK;
}

bool data_logger_is_enabled(void) {
    return logger_config.enabled;
}

esp_err_t data_logger_export_csv(char *buffer, size_t max_len, size_t *length) {
    if (!initialized || buffer == NULL || length == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t offset = 0;
    
    // Write CSV header
    offset += snprintf(buffer + offset, max_len - offset,
                       "Timestamp,Temperature,Humidity,Ammonia,CO2,Light,"
                       "Water_Level,Feed_Level,Exhaust_Fan,Inlet_Fan,"
                       "Heater,Feeder,Lighting,Ventilation,Alert\n");
    
    // Write log entries
    for (size_t i = 0; i < log_count && offset < max_len - 100; i++) {
        log_entry_t *entry = &log_entries[i];
        offset += snprintf(buffer + offset, max_len - offset,
                          "%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%u,%u,%u,%u,%u,%d\n",
                          entry->timestamp,
                          entry->sensor_data.temperature,
                          entry->sensor_data.humidity,
                          entry->sensor_data.ammonia_ppm,
                          entry->sensor_data.co2_ppm,
                          entry->sensor_data.light_lux,
                          entry->sensor_data.water_level_percent,
                          entry->sensor_data.feed_level_percent,
                          entry->actuator_states.exhaust_fan_speed,
                          entry->actuator_states.inlet_fan_speed,
                          entry->actuator_states.heater_power,
                          entry->actuator_states.feeder_speed,
                          entry->actuator_states.lighting_intensity,
                          entry->actuator_states.ventilation_position,
                          entry->active_alert);
    }
    
    *length = offset;
    return ESP_OK;
}

esp_err_t data_logger_deinit(void) {
    if (initialized) {
        // Save remaining entries to NVS
        nvs_set_blob(nvs_handle, "logs", log_entries, log_count * sizeof(log_entry_t));
        nvs_commit(nvs_handle);
        
        nvs_close(nvs_handle);
        initialized = false;
        ESP_LOGI(TAG, "Data logger deinitialized");
    }
    return ESP_OK;
}
