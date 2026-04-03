#ifndef AMMONIA_SENSOR_H
#define AMMONIA_SENSOR_H

#include "driver/adc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ammonia sensor configuration
 */
typedef struct {
    adc1_channel_t adc_channel;     /*!< ADC channel for sensor */
    float calibration_offset;       /*!< Calibration offset in PPM */
    float calibration_scale;        /*!< Calibration scale factor */
    uint16_t samples;               /*!< Number of samples for averaging */
} ammonia_sensor_config_t;

/**
 * @brief Initialize ammonia sensor
 * 
 * @param config Sensor configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ammonia_sensor_init(const ammonia_sensor_config_t *config);

/**
 * @brief Read ammonia concentration
 * 
 * @param ppm Pointer to store ammonia concentration in PPM
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ammonia_sensor_read(float *ppm);

/**
 * @brief Calibrate sensor with known concentration
 * 
 * @param known_ppm Known ammonia concentration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ammonia_sensor_calibrate(float known_ppm);

/**
 * @brief Get raw ADC value
 * 
 * @return int Raw ADC value (0-4095)
 */
int ammonia_sensor_get_raw(void);

/**
 * @brief Check if sensor is connected
 * 
 * @return true if sensor is responding
 */
bool ammonia_sensor_is_connected(void);

/**
 * @brief Deinitialize ammonia sensor
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ammonia_sensor_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // AMMONIA_SENSOR_H
