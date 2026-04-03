#ifndef FEED_LEVEL_SENSOR_H
#define FEED_LEVEL_SENSOR_H

#include "driver/adc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Feed level sensor configuration
 */
typedef struct {
    adc1_channel_t adc_channel;     /*!< ADC channel for sensor */
    float calibration_offset;       /*!< Calibration offset in percentage */
    float calibration_scale;        /*!< Calibration scale factor */
    uint16_t samples;               /*!< Number of samples for averaging */
    float hopper_capacity_kg;       /*!< Hopper capacity in kilograms */
} feed_level_config_t;

/**
 * @brief Initialize feed level sensor
 * 
 * @param config Sensor configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feed_level_sensor_init(const feed_level_config_t *config);

/**
 * @brief Read feed level
 * 
 * @param percent Pointer to store feed level in percentage
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feed_level_sensor_read(float *percent);

/**
 * @brief Read feed level in kilograms
 * 
 * @param kg Pointer to store feed level in kilograms
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feed_level_sensor_read_kg(float *kg);

/**
 * @brief Calibrate sensor with known level
 * 
 * @param known_percent Known feed level percentage
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feed_level_sensor_calibrate(float known_percent);

/**
 * @brief Get raw ADC value
 * 
 * @return int Raw ADC value (0-4095)
 */
int feed_level_sensor_get_raw(void);

/**
 * @brief Check if sensor is connected
 * 
 * @return true if sensor is responding
 */
bool feed_level_sensor_is_connected(void);

/**
 * @brief Deinitialize feed level sensor
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feed_level_sensor_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // FEED_LEVEL_SENSOR_H
