#include "dht22.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include <string.h>

static const char *TAG = "DHT22";

#define DHT22_MIN_INTERVAL_US   2000000  // 2 seconds minimum between reads
#define DHT22_TIMEOUT_US        1000     // 1ms timeout for bit reading

static gpio_num_t dht22_gpio = GPIO_NUM_NC;
static dht22_data_t last_data = {0};
static uint64_t last_read_time = 0;

/**
 * @brief Wait for pin to reach expected state with timeout
 */
static int wait_for_pin(gpio_num_t gpio, int expected_value, uint32_t timeout_us) {
    uint32_t start = esp_timer_get_time();
    while (gpio_get_level(gpio) != expected_value) {
        if ((esp_timer_get_time() - start) > timeout_us) {
            return -1;
        }
        ets_delay_us(1);
    }
    return 0;
}

/**
 * @brief Read raw data from DHT22
 */
static esp_err_t dht22_read_raw(uint8_t data[5]) {
    uint8_t bits[40];
    int i;
    
    // Check minimum interval
    uint64_t now = esp_timer_get_time();
    uint64_t elapsed_us = now - last_read_time;
    if (elapsed_us < DHT22_MIN_INTERVAL_US) {
        uint32_t delay_ms = (DHT22_MIN_INTERVAL_US - elapsed_us) / 1000;
        if (delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
    
    // Send start signal
    gpio_set_direction(dht22_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht22_gpio, 0);
    ets_delay_us(1000);  // 1ms low
    gpio_set_level(dht22_gpio, 1);
    ets_delay_us(30);    // 30us high
    
    // Switch to input mode
    gpio_set_direction(dht22_gpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode(dht22_gpio, GPIO_PULLUP_ONLY);
    
    // Wait for DHT22 response
    if (wait_for_pin(dht22_gpio, 0, 100) != 0) {
        ESP_LOGE(TAG, "No response from sensor (step 1)");
        return ESP_ERR_TIMEOUT;
    }
    if (wait_for_pin(dht22_gpio, 1, 100) != 0) {
        ESP_LOGE(TAG, "No response from sensor (step 2)");
        return ESP_ERR_TIMEOUT;
    }
    if (wait_for_pin(dht22_gpio, 0, 100) != 0) {
        ESP_LOGE(TAG, "No response from sensor (step 3)");
        return ESP_ERR_TIMEOUT;
    }
    
    // Read 40 bits
    for (i = 0; i < 40; i++) {
        // Wait for high pulse
        if (wait_for_pin(dht22_gpio, 1, DHT22_TIMEOUT_US) != 0) {
            ESP_LOGE(TAG, "Timeout reading bit %d", i);
            return ESP_ERR_TIMEOUT;
        }
        
        // Measure high pulse duration
        uint32_t start = esp_timer_get_time();
        if (wait_for_pin(dht22_gpio, 0, DHT22_TIMEOUT_US) != 0) {
            ESP_LOGE(TAG, "Timeout reading bit %d (low)", i);
            return ESP_ERR_TIMEOUT;
        }
        uint32_t duration = esp_timer_get_time() - start;
        
        // Bit is 1 if high pulse > 40us
        bits[i] = (duration > 40) ? 1 : 0;
    }
    
    // Convert bits to bytes
    memset(data, 0, 5);
    for (i = 0; i < 40; i++) {
        data[i / 8] <<= 1;
        data[i / 8] |= bits[i];
    }
    
    // Verify checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGE(TAG, "Checksum error: calculated=%d, received=%d", checksum, data[4]);
        return ESP_ERR_INVALID_CRC;
    }
    
    last_read_time = esp_timer_get_time();
    return ESP_OK;
}

esp_err_t dht22_init(gpio_num_t gpio_num) {
    if (gpio_num == GPIO_NUM_NC) {
        return ESP_ERR_INVALID_ARG;
    }
    
    dht22_gpio = gpio_num;
    
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Initialize data
    memset(&last_data, 0, sizeof(last_data));
    last_read_time = 0;
    
    ESP_LOGI(TAG, "DHT22 initialized on GPIO %d", gpio_num);
    return ESP_OK;
}

esp_err_t dht22_read(dht22_data_t *data) {
    if (dht22_gpio == GPIO_NUM_NC || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t raw_data[5];
    esp_err_t ret = dht22_read_raw(raw_data);
    
    if (ret != ESP_OK) {
        data->valid = false;
        return ret;
    }
    
    // Parse humidity (bytes 0-1)
    uint16_t raw_humidity = (raw_data[0] << 8) | raw_data[1];
    data->humidity = raw_humidity / 10.0f;
    
    // Parse temperature (bytes 2-3)
    uint16_t raw_temp = (raw_data[2] << 8) | raw_data[3];
    if (raw_temp & 0x8000) {
        // Negative temperature
        raw_temp &= 0x7FFF;
        data->temperature = -(raw_temp / 10.0f);
    } else {
        data->temperature = raw_temp / 10.0f;
    }
    
    // Validate ranges
    if (data->humidity < 0 || data->humidity > 100 ||
        data->temperature < -40 || data->temperature > 80) {
        ESP_LOGW(TAG, "Sensor data out of range: T=%.1f, H=%.1f", 
                 data->temperature, data->humidity);
        data->valid = false;
        return ESP_ERR_INVALID_STATE;
    }
    
    data->valid = true;
    data->timestamp = esp_timer_get_time() / 1000000; // Convert to seconds
    
    // Update last valid data
    memcpy(&last_data, data, sizeof(dht22_data_t));
    
    ESP_LOGD(TAG, "Temperature: %.1f°C, Humidity: %.1f%%", 
             data->temperature, data->humidity);
    
    return ESP_OK;
}

float dht22_get_temperature(void) {
    return last_data.temperature;
}

float dht22_get_humidity(void) {
    return last_data.humidity;
}

bool dht22_is_connected(void) {
    dht22_data_t test_data;
    return (dht22_read(&test_data) == ESP_OK && test_data.valid);
}

esp_err_t dht22_deinit(void) {
    dht22_gpio = GPIO_NUM_NC;
    memset(&last_data, 0, sizeof(last_data));
    ESP_LOGI(TAG, "DHT22 deinitialized");
    return ESP_OK;
}
