#ifndef CO2_SENSOR_H
#define CO2_SENSOR_H

#include "driver/adc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CO2 sensor configuration
 */
typedef struct {
    adc1_channel_t adc_channel;     /*!< ADC channel for sensor */
    float calibration_offset;       /*!< Calibration offset in PPM */
    float calibration_scale;        /*!< Calibration scale factor */
    uint16_t samples;               /*!< Number of samples for averaging */
} co2_sensor_config_t;

/**
 * @brief Initialize CO2 sensor
 * 
 * @param config Sensor configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t co2_sensor_init(const co2_sensor_config_t *config);

/**
 * @brief Read CO2 concentration
 * 
 * @param ppm Pointer to store CO2 concentration in PPM
 * @return esp_err_t ESP_OK on success
 */
esp_err_t co2_sensor_read(float *ppm);

/**
 * @brief Calibrate sensor with known concentration
 * 
 * @param known_ppm Known CO2 concentration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t co2_sensor_calibrate(float known_ppm);

/**
 * @brief Get raw ADC value
 * 
 * @return int Raw ADC value (0-4095)
 */
int co2_sensor_get_raw(void);

/**
 * @brief Check if sensor is connected
 * 
 * @return true if sensor is responding
 */
bool co2_sensor_is_connected(void);

/**
 * @brief Deinitialize CO2 sensor
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t co2_sensor_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // CO2_SENSOR_H
