#ifndef GAS_ALERT_CONFIG_H
#define GAS_ALERT_CONFIG_H

#include "poultry_system_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Gas Sensor Configuration
#define GAS_SENSOR_ADC_CHANNEL      ADC1_CHANNEL_5  // GPIO33
#define GAS_SENSOR_TYPE             GAS_TYPE_METHANE  // MQ-4 for methane/LPG

// Gas Leak Thresholds (in PPM)
#define GAS_LEAK_LOW_THRESHOLD      1000.0f   // 1000 ppm - Low level alert
#define GAS_LEAK_MODERATE_THRESHOLD 2500.0f   // 2500 ppm - Moderate alert
#define GAS_LEAK_HIGH_THRESHOLD     5000.0f   // 5000 ppm - High level alert
#define GAS_LEAK_CRITICAL_THRESHOLD 10000.0f  // 10000 ppm - Critical alert

// Gas Leak Detection Settings
#define GAS_LEAK_DETECTION_INTERVAL_MS  1000    // Check every 1 second
#define GAS_LEAK_BUZZER_ENABLED         true    // Enable buzzer alerts
#define GAS_LEAK_MQTT_ENABLED           true    // Enable MQTT notifications
#define GAS_LEAK_AUTO_VENTILATION       true    // Enable auto ventilation on leak

// Buzzer Configuration
#define BUZZER_GPIO                 GPIO_NUM_25  // Buzzer GPIO pin
#define BUZZER_ACTIVE_HIGH          true         // Active high buzzer
#define BUZZER_FREQUENCY_HZ         2000         // 2kHz frequency

// Alert Types for Gas Leak
#define ALERT_GAS_LEAK              ALERT_SYSTEM_ERROR + 1  // Custom alert type

// Gas Sensor Calibration
#define GAS_SENSOR_CALIBRATION_OFFSET   0.0f
#define GAS_SENSOR_CALIBRATION_SCALE    1.0f
#define GAS_SENSOR_SAMPLES              64

// Gas Sensor Alarm Thresholds
#define GAS_SENSOR_ALARM_THRESHOLD      1000.0f  // 1000 ppm
#define GAS_SENSOR_DANGER_THRESHOLD     5000.0f  // 5000 ppm

#ifdef __cplusplus
}
#endif

#endif // GAS_ALERT_CONFIG_H
