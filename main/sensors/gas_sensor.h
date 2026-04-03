#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include "driver/adc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gas sensor types
 */
typedef enum {
    GAS_TYPE_METHANE = 0,       /*!< Methane (CH4) */
    GAS_TYPE_LPG,               /*!< Liquefied Petroleum Gas */
    GAS_TYPE_CARBON_MONOXIDE,   /*!< Carbon Monoxide (CO) */
    GAS_TYPE_SMOKE,             /*!< Smoke */
    GAS_TYPE_GENERAL            /*!< General combustible gas */
} gas_type_t;

/**
 * @brief Gas sensor configuration
 */
typedef struct {
    adc1_channel_t adc_channel;     /*!< ADC channel for sensor */
    gas_type_t gas_type;            /*!< Type of gas to detect */
    float calibration_offset;       /*!< Calibration offset in PPM */
    float calibration_scale;        /*!< Calibration scale factor */
    uint16_t samples;               /*!< Number of samples for averaging */
    float alarm_threshold_ppm;      /*!< Alarm threshold in PPM */
    float danger_threshold_ppm;     /*!< Danger threshold in PPM */
} gas_sensor_config_t;

/**
 * @brief Gas sensor data structure
 */
typedef struct {
    float concentration_ppm;        /*!< Gas concentration in PPM */
    float percentage;               /*!< Gas percentage of LEL */
    bool alarm_triggered;           /*!< Alarm status */
    bool danger_triggered;          /*!< Danger status */
    uint32_t timestamp;             /*!< Reading timestamp */
    gas_type_t gas_type;            /*!< Type of gas detected */
} gas_sensor_data_t;

/**
 * @brief Initialize gas sensor
 * 
 * @param config Sensor configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_sensor_init(const gas_sensor_config_t *config);

/**
 * @brief Read gas concentration
 * 
 * @param data Pointer to store gas sensor data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_sensor_read(gas_sensor_data_t *data);

/**
 * @brief Read gas concentration in PPM
 * 
 * @param ppm Pointer to store gas concentration in PPM
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_sensor_read_ppm(float *ppm);

/**
 * @brief Check if gas leak is detected
 * 
 * @param is_leak Pointer to store leak status
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_sensor_check_leak(bool *is_leak);

/**
 * @brief Set alarm threshold
 * 
 * @param threshold_ppm Alarm threshold in PPM
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_sensor_set_alarm_threshold(float threshold_ppm);

/**
 * @brief Set danger threshold
 * 
 * @param threshold_ppm Danger threshold in PPM
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_sensor_set_danger_threshold(float threshold_ppm);

/**
 * @brief Calibrate sensor with known concentration
 * 
 * @param known_ppm Known gas concentration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_sensor_calibrate(float known_ppm);

/**
 * @brief Get raw ADC value
 * 
 * @return int Raw ADC value (0-4095)
 */
int gas_sensor_get_raw(void);

/**
 * @brief Check if sensor is connected
 * 
 * @return true if sensor is responding
 */
bool gas_sensor_is_connected(void);

/**
 * @brief Get gas type name as string
 * 
 * @param gas_type Gas type
 * @return const char* Gas type name
 */
const char* gas_sensor_get_type_name(gas_type_t gas_type);

/**
 * @brief Deinitialize gas sensor
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t gas_sensor_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // GAS_SENSOR_H
