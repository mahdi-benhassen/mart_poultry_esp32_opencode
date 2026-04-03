#ifndef POULTRY_SYSTEM_CONFIG_H
#define POULTRY_SYSTEM_CONFIG_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <stdbool.h>
#include <stdint.h>

// System Version
#define SYSTEM_VERSION "1.0.0"
#define SYSTEM_NAME "SmartPoultry Pro"

// GPIO Pin Definitions - Sensors
#define DHT22_GPIO_PIN              GPIO_NUM_4
#define AMMONIA_SENSOR_ADC_CHANNEL  ADC1_CHANNEL_0  // GPIO 36
#define CO2_SENSOR_ADC_CHANNEL     ADC1_CHANNEL_3  // GPIO 39 (FIXED: was ADC1_CHANNEL_1)
#define LIGHT_SENSOR_ADC_CHANNEL   ADC1_CHANNEL_6  // GPIO 34 (FIXED: was ADC1_CHANNEL_2)
#define WATER_LEVEL_ADC_CHANNEL    ADC1_CHANNEL_7  // GPIO 35
#define FEED_LEVEL_ADC_CHANNEL     ADC1_CHANNEL_4  // GPIO 32
#define GAS_SENSOR_ADC_CHANNEL     ADC1_CHANNEL_5  // GPIO 33
#define GAS_SENSOR_TYPE            GAS_TYPE_GENERAL
#define GAS_SENSOR_CALIBRATION_OFFSET 0.0f
#define GAS_SENSOR_CALIBRATION_SCALE  1.0f
#define GAS_SENSOR_SAMPLES         64
#define GAS_SENSOR_ALARM_THRESHOLD  1000.0f
#define GAS_SENSOR_DANGER_THRESHOLD 5000.0f

// GPIO Pin Definitions - Actuators
#define EXHAUST_FAN_GPIO            GPIO_NUM_16
#define INLET_FAN_GPIO              GPIO_NUM_17
#define HEATER_GPIO                 GPIO_NUM_18
// Reserved for future use: COOLER_GPIO (GPIO_NUM_19)
// #define COOLER_GPIO                 GPIO_NUM_19
#define FEEDER_MOTOR_GPIO           GPIO_NUM_21
#define WATER_PUMP_GPIO             GPIO_NUM_22
#define LIGHTING_GPIO               GPIO_NUM_23
#define ALARM_GPIO                  GPIO_NUM_25
#define VENTILATION_SERVO_GPIO      GPIO_NUM_26
#define CURTAIN_MOTOR_GPIO          GPIO_NUM_27

// PWM Channels
#define EXHAUST_FAN_PWM_CHANNEL     LEDC_CHANNEL_0
#define INLET_FAN_PWM_CHANNEL       LEDC_CHANNEL_1
#define HEATER_PWM_CHANNEL          LEDC_CHANNEL_2
#define FEEDER_PWM_CHANNEL          LEDC_CHANNEL_3
#define LIGHTING_PWM_CHANNEL        LEDC_CHANNEL_4
#define VENTILATION_PWM_CHANNEL     LEDC_CHANNEL_5
#define VENTILATION_PWM_TIMER       LEDC_TIMER_3

// PWM Configuration
#define PWM_FREQUENCY               5000
#define PWM_RESOLUTION              LEDC_TIMER_10_BIT
#define PWM_MAX_DUTY                1023

// Sensor Thresholds
#define TEMP_MIN_THRESHOLD          20.0f   // Celsius
#define TEMP_MAX_THRESHOLD          32.0f   // Celsius
#define TEMP_OPTIMAL_MIN            24.0f   // Celsius
#define TEMP_OPTIMAL_MAX            28.0f   // Celsius

#define HUMIDITY_MIN_THRESHOLD      40.0f   // Percentage
#define HUMIDITY_MAX_THRESHOLD      70.0f   // Percentage
#define HUMIDITY_OPTIMAL_MIN        50.0f   // Percentage
#define HUMIDITY_OPTIMAL_MAX        65.0f   // Percentage

#define AMMONIA_MAX_THRESHOLD       25.0f   // PPM
#define AMMONIA_WARNING_THRESHOLD   20.0f   // PPM

#define CO2_MAX_THRESHOLD           3000.0f // PPM
#define CO2_WARNING_THRESHOLD       2500.0f // PPM

#define LIGHT_INTENSITY_MIN         20.0f   // Lux
#define LIGHT_INTENSITY_MAX         50.0f   // Lux

#define WATER_LEVEL_MIN             20.0f   // Percentage
#define FEED_LEVEL_MIN              15.0f   // Percentage

// PID Controller Parameters
#define PID_TEMP_KP                 2.0f
#define PID_TEMP_KI                 0.5f
#define PID_TEMP_KD                 1.0f

#define PID_HUMIDITY_KP             1.5f
#define PID_HUMIDITY_KI             0.3f
#define PID_HUMIDITY_KD             0.8f

#define PID_VENTILATION_KP          1.0f
#define PID_VENTILATION_KI          0.2f
#define PID_VENTILATION_KD          0.5f

// Timing Constants
#define SENSOR_READ_INTERVAL_MS     5000    // 5 seconds
#define CONTROL_LOOP_INTERVAL_MS    10000   // 10 seconds
#define DATA_LOG_INTERVAL_MS        60000   // 1 minute
#define MQTT_PUBLISH_INTERVAL_MS    30000   // 30 seconds
#define WEB_UPDATE_INTERVAL_MS      2000    // 2 seconds

// Task Stack Sizes
#define SENSOR_TASK_STACK_SIZE      8192
#define CONTROL_TASK_STACK_SIZE     8192
#define MQTT_TASK_STACK_SIZE        8192
#define WEB_SERVER_TASK_STACK_SIZE  8192
#define DATA_LOGGER_TASK_STACK_SIZE 8192

// Task Priorities
#define SENSOR_TASK_PRIORITY        5
#define CONTROL_TASK_PRIORITY       6
#define MQTT_TASK_PRIORITY          4
#define WEB_SERVER_TASK_PRIORITY    3
#define DATA_LOGGER_TASK_PRIORITY   2

// WiFi Configuration
#define WIFI_SSID_MAX_LEN           32
#define WIFI_PASSWORD_MAX_LEN       64
#define WIFI_RETRY_MAX              5
#define WIFI_CONNECT_TIMEOUT_MS     10000

// MQTT Configuration
#define MQTT_BROKER_MAX_LEN         128
#define MQTT_TOPIC_MAX_LEN          64
#define MQTT_CLIENT_ID_MAX_LEN      32
#define MQTT_KEEPALIVE              120
#define MQTT_QOS                    1

// Web Server Configuration
#define WEB_SERVER_PORT             80
#define WEB_MAX_CONNECTIONS         4
#define WEB_STACK_SIZE              8192

// Data Storage
#define NVS_NAMESPACE               "poultry_cfg"
#define LOG_FILE_MAX_SIZE           1048576  // 1MB
#define MAX_LOG_FILES               10

// Alert Types
typedef enum {
    ALERT_NONE = 0,
    ALERT_TEMP_HIGH,
    ALERT_TEMP_LOW,
    ALERT_HUMIDITY_HIGH,
    ALERT_HUMIDITY_LOW,
    ALERT_AMMONIA_HIGH,
    ALERT_CO2_HIGH,
    ALERT_WATER_LOW,
    ALERT_FEED_LOW,
    ALERT_GAS_LEAK,
    ALERT_SENSOR_FAILURE,
    ALERT_ACTUATOR_FAILURE,
    ALERT_SYSTEM_ERROR
} alert_type_t;

// System States
typedef enum {
    SYSTEM_STATE_INIT = 0,
    SYSTEM_STATE_NORMAL,
    SYSTEM_STATE_WARNING,
    SYSTEM_STATE_CRITICAL,
    SYSTEM_STATE_MAINTENANCE,
    SYSTEM_STATE_ERROR
} system_state_t;

// Operating Modes
typedef enum {
    MODE_AUTOMATIC = 0,
    MODE_MANUAL,
    MODE_SCHEDULED,
    MODE_MAINTENANCE
} operating_mode_t;

// Sensor Data Structure
typedef struct {
    float temperature;
    float humidity;
    float ammonia_ppm;
    float co2_ppm;
    float light_lux;
    float water_level_percent;
    float feed_level_percent;
    float gas_ppm;
    bool gas_alarm;
    uint32_t timestamp;
    bool valid;
} sensor_data_t;

// Actuator States Structure
typedef struct {
    uint8_t exhaust_fan_speed;      // 0-100%
    uint8_t inlet_fan_speed;        // 0-100%
    uint8_t heater_power;           // 0-100%
    // Reserved for future use: uint8_t cooler_power;
    uint8_t feeder_speed;           // 0-100%
    uint8_t water_pump_state;       // 0 or 1
    uint8_t lighting_intensity;     // 0-100%
    uint8_t ventilation_position;   // 0-100%
    uint8_t curtain_position;       // 0-100%
    bool alarm_state;
} actuator_states_t;

// System Configuration Structure
typedef struct {
    char wifi_ssid[WIFI_SSID_MAX_LEN];
    char wifi_password[WIFI_PASSWORD_MAX_LEN];
    char mqtt_broker[MQTT_BROKER_MAX_LEN];
    uint16_t mqtt_port;
    char mqtt_username[32];
    char mqtt_password[32];
    char mqtt_client_id[MQTT_CLIENT_ID_MAX_LEN];
    
    float temp_setpoint;
    float humidity_setpoint;
    float ammonia_limit;
    float co2_limit;
    
    operating_mode_t operating_mode;
    bool alerts_enabled;
    bool data_logging_enabled;
    bool mqtt_enabled;
    
    uint8_t feeding_schedule[24];   // Hourly feeding amounts
    uint8_t lighting_schedule[24];  // Hourly lighting levels
    
    uint32_t config_version;
} system_config_t;

// System Status Structure
typedef struct {
    system_state_t state;
    sensor_data_t current_sensor_data;
    actuator_states_t actuator_states;
    alert_type_t active_alert;
    uint32_t uptime_seconds;
    uint32_t free_heap_size;
    uint32_t total_log_entries;
    char last_error[128];
} system_status_t;

#endif // POULTRY_SYSTEM_CONFIG_H
