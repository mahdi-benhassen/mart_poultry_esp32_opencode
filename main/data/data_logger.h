#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include "esp_err.h"
#include "../include/poultry_system_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Data logger configuration
 */
typedef struct {
    bool enabled;                   /*!< Enable/disable data logging */
    uint32_t log_interval_ms;       /*!< Logging interval in milliseconds */
    uint32_t max_file_size;         /*!< Maximum log file size in bytes */
    uint8_t max_files;              /*!< Maximum number of log files */
} data_logger_config_t;

/**
 * @brief Log entry structure
 */
typedef struct {
    uint32_t timestamp;             /*!< Entry timestamp */
    sensor_data_t sensor_data;      /*!< Sensor data */
    actuator_states_t actuator_states; /*!< Actuator states */
    alert_type_t active_alert;      /*!< Active alert type */
} log_entry_t;

/**
 * @brief Initialize data logger
 * 
 * @param config Logger configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t data_logger_init(const data_logger_config_t *config);

/**
 * @brief Log sensor data
 * 
 * @param sensor_data Sensor data to log
 * @param actuator_states Actuator states to log
 * @param active_alert Active alert type
 * @return esp_err_t ESP_OK on success
 */
esp_err_t data_logger_log(const sensor_data_t *sensor_data, 
                          const actuator_states_t *actuator_states,
                          alert_type_t active_alert);

/**
 * @brief Get log entries
 * 
 * @param entries Array to store log entries
 * @param max_count Maximum number of entries to retrieve
 * @param count Pointer to store actual number of entries retrieved
 * @return esp_err_t ESP_OK on success
 */
esp_err_t data_logger_get_entries(log_entry_t *entries, size_t max_count, 
                                  size_t *count);

/**
 * @brief Get log entries for time range
 * 
 * @param start_time Start timestamp
 * @param end_time End timestamp
 * @param entries Array to store log entries
 * @param max_count Maximum number of entries to retrieve
 * @param count Pointer to store actual number of entries retrieved
 * @return esp_err_t ESP_OK on success
 */
esp_err_t data_logger_get_entries_range(uint32_t start_time, uint32_t end_time,
                                        log_entry_t *entries, size_t max_count, 
                                        size_t *count);

/**
 * @brief Clear log entries
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t data_logger_clear(void);

/**
 * @brief Get total log entries count
 * 
 * @return size_t Total number of log entries
 */
size_t data_logger_get_count(void);

/**
 * @brief Enable/disable data logger
 * 
 * @param enabled true to enable, false to disable
 * @return esp_err_t ESP_OK on success
 */
esp_err_t data_logger_enable(bool enabled);

/**
 * @brief Check if data logger is enabled
 * 
 * @return true if enabled
 */
bool data_logger_is_enabled(void);

/**
 * @brief Export log to CSV format
 * 
 * @param buffer Buffer to store CSV data
 * @param max_len Maximum length of buffer
 * @param length Pointer to store actual length
 * @return esp_err_t ESP_OK on success
 */
esp_err_t data_logger_export_csv(char *buffer, size_t max_len, size_t *length);

/**
 * @brief Deinitialize data logger
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t data_logger_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // DATA_LOGGER_H
