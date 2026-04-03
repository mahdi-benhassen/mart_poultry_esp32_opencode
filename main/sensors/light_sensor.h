#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "driver/adc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Light sensor configuration
 */
typedef struct {
    adc1_channel_t adc_channel;     /*!< ADC channel for sensor */
    float calibration_offset;       /*!< Calibration offset in Lux */
    float calibration_scale;        /*!< Calibration scale factor */
    uint16_t samples;               /*!< Number of samples for averaging */
} light_sensor_config_t;

/**
 * @brief Initialize light sensor
 * 
 * @param config Sensor configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t light_sensor_init(const light_sensor_config_t *config);

/**
 * @brief Read light intensity
 * 
 * @param lux Pointer to store light intensity in Lux
 * @return esp_err_t ESP_OK on success
 */
esp_err_t light_sensor_read(float *lux);

/**
 * @brief Calibrate sensor with known intensity
 * 
 * @param known_lux Known light intensity
 * @return esp_err_t ESP_OK on success
 */
esp_err_t light_sensor_calibrate(float known_lux);

/**
 * @brief Get raw ADC value
 * 
 * @return int Raw ADC value (0-4095)
 */
int light_sensor_get_raw(void);

/**
 * @brief Check if sensor is connected
 * 
 * @return true if sensor is responding
 */
bool light_sensor_is_connected(void);

/**
 * @brief Deinitialize light sensor
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t light_sensor_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // LIGHT_SENSOR_H
