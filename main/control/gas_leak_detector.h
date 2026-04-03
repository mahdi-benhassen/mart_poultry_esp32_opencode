#ifndef GAS_LEAK_DETECTOR_H
#define GAS_LEAK_DETECTOR_H

#include "esp_err.h"
#include "../include/poultry_system_config.h"
#include "../sensors/gas_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gas leak severity levels
 */
typedef enum {
    GAS_LEAK_NONE = 0,          /*!< No gas leak detected */
    GAS_LEAK_LOW,               /*!< Low level gas leak */
    GAS_LEAK_MODERATE,          /*!< Moderate gas leak */
    GAS_LEAK_HIGH,              /*!< High level gas leak */
    GAS_LEAK_CRITICAL           /*!< Critical gas leak - immediate danger */
} gas_leak_severity_t;

/**
 * @brief Gas leak detector configuration
 */
typedef struct {
    bool enabled;                   /*!< Enable/disable gas leak detection */
    float low_threshold_ppm;        /*!< Low level threshold in PPM */
    float moderate_threshold_ppm;   /*!< Moderate level threshold in PPM */
    float high_threshold_ppm;       /*!< High level threshold in PPM */
    float critical_threshold_ppm;   /*!< Critical level threshold in PPM */
    uint32_t detection_interval_ms; /*!< Detection interval in milliseconds */
    bool buzzer_enabled;            /*!< Enable buzzer alerts */
    bool mqtt_enabled;              /*!< Enable MQTT notifications */
    bool auto_ventilation;          /*!< Enable automatic ventilation on leak */
} gas_leak_config_t;

/**
 * @brief Gas leak event structure
 */
typedef struct {
    gas_leak_severity_t severity;   /*!< Leak severity */
    gas_type_t gas_type;            /*!< Type of gas detected */
    float concentration_ppm;        /*!< Gas concentration in PPM */
    float percentage_lel;           /*!< Percentage of LEL */
    uint32_t timestamp;             /*!< Event timestamp */
    char message[128];              /*!< Event message */
} gas_leak_event_t;

/**
 * @brief Initialize gas leak detector
 * 
 * @param config Detector configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_leak_detector_init(const gas_leak_config_t *config);

/**
 * @brief Check for gas leaks
 * 
 * @param event Pointer to store gas leak event if detected
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_leak_detector_check(gas_leak_event_t *event);

/**
 * @brief Get current gas leak severity
 * 
 * @return gas_leak_severity_t Current severity level
 */
gas_leak_severity_t gas_leak_detector_get_severity(void);

/**
 * @brief Get gas leak event history
 * 
 * @param events Array to store events
 * @param max_count Maximum number of events to retrieve
 * @param count Pointer to store actual number of events retrieved
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_leak_detector_get_history(gas_leak_event_t *events, size_t max_count, 
                                        size_t *count);

/**
 * @brief Clear gas leak event history
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_leak_detector_clear_history(void);

/**
 * @brief Set gas leak thresholds
 * 
 * @param low Low threshold in PPM
 * @param moderate Moderate threshold in PPM
 * @param high High threshold in PPM
 * @param critical Critical threshold in PPM
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_leak_detector_set_thresholds(float low, float moderate, 
                                           float high, float critical);

/**
 * @brief Enable/disable gas leak detection
 * 
 * @param enabled true to enable, false to disable
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_leak_detector_enable(bool enabled);

/**
 * @brief Check if gas leak detection is enabled
 * 
 * @return true if enabled
 */
bool gas_leak_detector_is_enabled(void);

/**
 * @brief Trigger manual gas leak alert
 * 
 * @param severity Leak severity
 * @param message Alert message
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_leak_detector_trigger_alert(gas_leak_severity_t severity, 
                                          const char *message);

/**
 * @brief Deinitialize gas leak detector
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_leak_detector_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // GAS_LEAK_DETECTOR_H
