#ifndef DHT22_H
#define DHT22_H

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DHT22 sensor data structure
 */
typedef struct {
    float temperature;      /*!< Temperature in Celsius */
    float humidity;         /*!< Relative humidity in percentage */
    bool valid;             /*!< Data validity flag */
    uint32_t timestamp;     /*!< Timestamp of last reading */
} dht22_data_t;

/**
 * @brief Initialize DHT22 sensor
 * 
 * @param gpio_num GPIO pin number connected to DHT22 data pin
 * @return esp_err_t ESP_OK on success
 */
esp_err_t dht22_init(gpio_num_t gpio_num);

/**
 * @brief Read data from DHT22 sensor
 * 
 * @param data Pointer to store sensor data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t dht22_read(dht22_data_t *data);

/**
 * @brief Get last valid temperature reading
 * 
 * @return float Temperature in Celsius
 */
float dht22_get_temperature(void);

/**
 * @brief Get last valid humidity reading
 * 
 * @return float Humidity in percentage
 */
float dht22_get_humidity(void);

/**
 * @brief Check if sensor is responding
 * 
 * @return true if sensor is responding
 */
bool dht22_is_connected(void);

/**
 * @brief Deinitialize DHT22 sensor
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t dht22_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // DHT22_H
