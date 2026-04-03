#ifndef WATER_LEVEL_SENSOR_H
#define WATER_LEVEL_SENSOR_H

#include "driver/adc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Water level sensor configuration
 */
typedef struct {
    adc1_channel_t adc_channel;     /*!< ADC channel for sensor */
    float calibration_offset;       /*!< Calibration offset in percentage */
    float calibration_scale;        /*!< Calibration scale factor */
    uint16_t samples;               /*!< Number of samples for averaging */
    float tank_height_cm;           /*!< Tank height in centimeters */
} water_level_config_t;

/**
 * @brief Initialize water level sensor
 * 
 * @param config Sensor configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_level_sensor_init(const water_level_config_t *config);

/**
 * @brief Read water level
 * 
 * @param percent Pointer to store water level in percentage
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_level_sensor_read(float *percent);

/**
 * @brief Read water level in centimeters
 * 
 * @param cm Pointer to store water level in centimeters
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_level_sensor_read_cm(float *cm);

/**
 * @brief Calibrate sensor with known level
 * 
 * @param known_percent Known water level percentage
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_level_sensor_calibrate(float known_percent);

/**
 * @brief Get raw ADC value
 * 
 * @return int Raw ADC value (0-4095)
 */
int water_level_sensor_get_raw(void);

/**
 * @brief Check if sensor is connected
 * 
 * @return true if sensor is responding
 */
bool water_level_sensor_is_connected(void);

/**
 * @brief Deinitialize water level sensor
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t water_level_sensor_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // WATER_LEVEL_SENSOR_H
