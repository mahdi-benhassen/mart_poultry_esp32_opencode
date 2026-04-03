#include "sensor_manager.h"
#include "dht22.h"
#include "ammonia_sensor.h"
#include "co2_sensor.h"
#include "light_sensor.h"
#include "water_level_sensor.h"
#include "feed_level_sensor.h"
#include "gas_sensor.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SENSOR_MGR";

// Sensor health bitmask
#define SENSOR_DHT22_OK         (1 << 0)
#define SENSOR_AMMONIA_OK       (1 << 1)
#define SENSOR_CO2_OK           (1 << 2)
#define SENSOR_LIGHT_OK         (1 << 3)
#define SENSOR_WATER_OK         (1 << 4)
#define SENSOR_FEED_OK          (1 << 5)
#define SENSOR_GAS_OK           (1 << 6)

static uint8_t sensor_health = 0;
static uint8_t sensor_failure_count[7] = {0};
#define SENSOR_FAILURE_THRESHOLD 5
static bool initialized = false;

esp_err_t sensor_manager_init(void) {
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing sensor manager...");
    
    // Initialize DHT22 sensor
    ret = dht22_init(DHT22_GPIO_PIN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize DHT22 sensor");
        sensor_health &= ~SENSOR_DHT22_OK;
    } else {
        sensor_health |= SENSOR_DHT22_OK;
        ESP_LOGI(TAG, "DHT22 sensor initialized");
    }
    
    // Initialize ammonia sensor
    ammonia_sensor_config_t ammonia_config = {
        .adc_channel = AMMONIA_SENSOR_ADC_CHANNEL,
        .calibration_offset = 0.0f,
        .calibration_scale = 1.0f,
        .samples = 64
    };
    ret = ammonia_sensor_init(&ammonia_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ammonia sensor");
        sensor_health &= ~SENSOR_AMMONIA_OK;
    } else {
        sensor_health |= SENSOR_AMMONIA_OK;
        ESP_LOGI(TAG, "Ammonia sensor initialized");
    }
    
    // Initialize CO2 sensor
    co2_sensor_config_t co2_config = {
        .adc_channel = CO2_SENSOR_ADC_CHANNEL,
        .calibration_offset = 0.0f,
        .calibration_scale = 1.0f,
        .samples = 64
    };
    ret = co2_sensor_init(&co2_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize CO2 sensor");
        sensor_health &= ~SENSOR_CO2_OK;
    } else {
        sensor_health |= SENSOR_CO2_OK;
        ESP_LOGI(TAG, "CO2 sensor initialized");
    }
    
    // Initialize light sensor
    light_sensor_config_t light_config = {
        .adc_channel = LIGHT_SENSOR_ADC_CHANNEL,
        .calibration_offset = 0.0f,
        .calibration_scale = 1.0f,
        .samples = 64
    };
    ret = light_sensor_init(&light_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize light sensor");
        sensor_health &= ~SENSOR_LIGHT_OK;
    } else {
        sensor_health |= SENSOR_LIGHT_OK;
        ESP_LOGI(TAG, "Light sensor initialized");
    }
    
    // Initialize water level sensor
    water_level_config_t water_config = {
        .adc_channel = WATER_LEVEL_ADC_CHANNEL,
        .calibration_offset = 0.0f,
        .calibration_scale = 1.0f,
        .samples = 64,
        .tank_height_cm = 100.0f
    };
    ret = water_level_sensor_init(&water_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize water level sensor");
        sensor_health &= ~SENSOR_WATER_OK;
    } else {
        sensor_health |= SENSOR_WATER_OK;
        ESP_LOGI(TAG, "Water level sensor initialized");
    }
    
    // Initialize feed level sensor
    feed_level_config_t feed_config = {
        .adc_channel = FEED_LEVEL_ADC_CHANNEL,
        .calibration_offset = 0.0f,
        .calibration_scale = 1.0f,
        .samples = 64,
        .hopper_capacity_kg = 50.0f
    };
    ret = feed_level_sensor_init(&feed_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize feed level sensor");
        sensor_health &= ~SENSOR_FEED_OK;
    } else {
        sensor_health |= SENSOR_FEED_OK;
        ESP_LOGI(TAG, "Feed level sensor initialized");
    }
    
    // Initialize gas sensor
    gas_sensor_config_t gas_config = {
        .adc_channel = GAS_SENSOR_ADC_CHANNEL,
        .gas_type = GAS_SENSOR_TYPE,
        .calibration_offset = GAS_SENSOR_CALIBRATION_OFFSET,
        .calibration_scale = GAS_SENSOR_CALIBRATION_SCALE,
        .samples = GAS_SENSOR_SAMPLES,
        .alarm_threshold_ppm = GAS_SENSOR_ALARM_THRESHOLD,
        .danger_threshold_ppm = GAS_SENSOR_DANGER_THRESHOLD
    };
    ret = gas_sensor_init(&gas_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize gas sensor");
        sensor_health &= ~SENSOR_GAS_OK;
    } else {
        sensor_health |= SENSOR_GAS_OK;
        ESP_LOGI(TAG, "Gas sensor initialized");
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Sensor manager initialized. Health mask: 0x%02X", sensor_health);
    
    if (sensor_health == 0) {
        ESP_LOGE(TAG, "No sensors initialized successfully");
        return ESP_ERR_NOT_FOUND;
    }
    
    return (sensor_health == (SENSOR_DHT22_OK | SENSOR_AMMONIA_OK | SENSOR_CO2_OK |
                              SENSOR_LIGHT_OK | SENSOR_WATER_OK | SENSOR_FEED_OK | SENSOR_GAS_OK))
           ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t sensor_manager_read_all(sensor_data_t *data) {
    if (!initialized || data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    memset(data, 0, sizeof(sensor_data_t));
    
    uint8_t failed_sensors = 0;
    uint8_t successful_reads = 0;
    
    if (sensor_health & SENSOR_DHT22_OK) {
        dht22_data_t dht_data;
        if (dht22_read(&dht_data) == ESP_OK && dht_data.valid) {
            data->temperature = dht_data.temperature;
            data->humidity = dht_data.humidity;
            successful_reads++;
            sensor_failure_count[0] = 0;
        } else {
            ESP_LOGW(TAG, "DHT22 read failed");
            sensor_failure_count[0]++;
            if (sensor_failure_count[0] >= SENSOR_FAILURE_THRESHOLD) {
                sensor_health &= ~SENSOR_DHT22_OK;
            }
            failed_sensors++;
        }
    } else {
        if (dht22_read(&dht_data) == ESP_OK && dht_data.valid) {
            sensor_health |= SENSOR_DHT22_OK;
            sensor_failure_count[0] = 0;
            ESP_LOGI(TAG, "DHT22 recovered");
        }
    }
    
    if (sensor_health & SENSOR_AMMONIA_OK) {
        if (ammonia_sensor_read(&data->ammonia_ppm) != ESP_OK) {
            ESP_LOGW(TAG, "Ammonia sensor read failed");
            sensor_failure_count[1]++;
            if (sensor_failure_count[1] >= SENSOR_FAILURE_THRESHOLD) {
                sensor_health &= ~SENSOR_AMMONIA_OK;
            }
            failed_sensors++;
        } else {
            successful_reads++;
            sensor_failure_count[1] = 0;
        }
    }
    
    if (sensor_health & SENSOR_CO2_OK) {
        if (co2_sensor_read(&data->co2_ppm) != ESP_OK) {
            ESP_LOGW(TAG, "CO2 sensor read failed");
            sensor_failure_count[2]++;
            if (sensor_failure_count[2] >= SENSOR_FAILURE_THRESHOLD) {
                sensor_health &= ~SENSOR_CO2_OK;
            }
            failed_sensors++;
        } else {
            successful_reads++;
            sensor_failure_count[2] = 0;
        }
    }
    
    if (sensor_health & SENSOR_LIGHT_OK) {
        if (light_sensor_read(&data->light_lux) != ESP_OK) {
            ESP_LOGW(TAG, "Light sensor read failed");
            sensor_failure_count[3]++;
            if (sensor_failure_count[3] >= SENSOR_FAILURE_THRESHOLD) {
                sensor_health &= ~SENSOR_LIGHT_OK;
            }
            failed_sensors++;
        } else {
            successful_reads++;
            sensor_failure_count[3] = 0;
        }
    }
    
    if (sensor_health & SENSOR_WATER_OK) {
        if (water_level_sensor_read(&data->water_level_percent) != ESP_OK) {
            ESP_LOGW(TAG, "Water level sensor read failed");
            sensor_failure_count[4]++;
            if (sensor_failure_count[4] >= SENSOR_FAILURE_THRESHOLD) {
                sensor_health &= ~SENSOR_WATER_OK;
            }
            failed_sensors++;
        } else {
            successful_reads++;
            sensor_failure_count[4] = 0;
        }
    }
    
    if (sensor_health & SENSOR_FEED_OK) {
        if (feed_level_sensor_read(&data->feed_level_percent) != ESP_OK) {
            ESP_LOGW(TAG, "Feed level sensor read failed");
            sensor_failure_count[5]++;
            if (sensor_failure_count[5] >= SENSOR_FAILURE_THRESHOLD) {
                sensor_health &= ~SENSOR_FEED_OK;
            }
            failed_sensors++;
        } else {
            successful_reads++;
            sensor_failure_count[5] = 0;
        }
    }
    
    if (sensor_health & SENSOR_GAS_OK) {
        gas_sensor_data_t gas_data;
        if (gas_sensor_read(&gas_data) == ESP_OK) {
            data->gas_ppm = gas_data.concentration_ppm;
            data->gas_alarm = gas_data.alarm_triggered;
            successful_reads++;
            sensor_failure_count[6] = 0;
        } else {
            ESP_LOGW(TAG, "Gas sensor read failed");
            sensor_failure_count[6]++;
            if (sensor_failure_count[6] >= SENSOR_FAILURE_THRESHOLD) {
                sensor_health &= ~SENSOR_GAS_OK;
            }
            failed_sensors++;
        }
    }
    
    data->timestamp = esp_timer_get_time() / 1000000;
    
    data->valid = (successful_reads > 0 && sensor_health != 0);
    
    if (failed_sensors > 0 && successful_reads == 0) {
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGD(TAG, "Sensor data: T=%.1f°C, H=%.1f%%, NH3=%.1fppm, CO2=%.1fppm, Light=%.1fLux, Water=%.1f%%, Feed=%.1f%%, Gas=%.1fppm",
             data->temperature, data->humidity, data->ammonia_ppm, data->co2_ppm,
             data->light_lux, data->water_level_percent, data->feed_level_percent,
             data->gas_ppm);
    
    return ESP_OK;
}

esp_err_t sensor_manager_read_sensor(uint8_t sensor_type, float *value) {
    if (!initialized || value == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    switch (sensor_type) {
        case 0: // Temperature
            if (sensor_health & SENSOR_DHT22_OK) {
                *value = dht22_get_temperature();
                return ESP_OK;
            }
            break;
            
        case 1: // Humidity
            if (sensor_health & SENSOR_DHT22_OK) {
                *value = dht22_get_humidity();
                return ESP_OK;
            }
            break;
            
        case 2: // Ammonia
            if (sensor_health & SENSOR_AMMONIA_OK) {
                return ammonia_sensor_read(value);
            }
            break;
            
        case 3: // CO2
            if (sensor_health & SENSOR_CO2_OK) {
                return co2_sensor_read(value);
            }
            break;
            
        case 4: // Light
            if (sensor_health & SENSOR_LIGHT_OK) {
                return light_sensor_read(value);
            }
            break;
            
        case 5: // Water level
            if (sensor_health & SENSOR_WATER_OK) {
                return water_level_sensor_read(value);
            }
            break;
            
        case 6: // Feed level
            if (sensor_health & SENSOR_FEED_OK) {
                return feed_level_sensor_read(value);
            }
            break;
            
        case 7: // Gas
            if (sensor_health & SENSOR_GAS_OK) {
                gas_sensor_data_t gas_data;
                esp_err_t ret = gas_sensor_read(&gas_data);
                if (ret == ESP_OK) {
                    *value = gas_data.concentration_ppm;
                }
                return ret;
            }
            break;
            
        default:
            return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_ERR_NOT_FOUND;
}

bool sensor_manager_check_all_connected(void) {
    if (!initialized) {
        return false;
    }
    
    return (sensor_health == (SENSOR_DHT22_OK | SENSOR_AMMONIA_OK | SENSOR_CO2_OK |
                              SENSOR_LIGHT_OK | SENSOR_WATER_OK | SENSOR_FEED_OK | SENSOR_GAS_OK));
}

esp_err_t sensor_manager_get_health(uint8_t *sensor_mask) {
    if (sensor_mask == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *sensor_mask = sensor_health;
    return ESP_OK;
}

esp_err_t sensor_manager_calibrate(uint8_t sensor_type, float known_value) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    switch (sensor_type) {
        case 2: // Ammonia
            return ammonia_sensor_calibrate(known_value);
            
        case 3: // CO2
            return co2_sensor_calibrate(known_value);
            
        case 4: // Light
            return light_sensor_calibrate(known_value);
            
        case 5: // Water level
            return water_level_sensor_calibrate(known_value);
            
        case 6: // Feed level
            return feed_level_sensor_calibrate(known_value);
            
        case 7: // Gas
            return gas_sensor_calibrate(known_value);
            
        default:
            return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t sensor_manager_deinit(void) {
    dht22_deinit();
    ammonia_sensor_deinit();
    co2_sensor_deinit();
    light_sensor_deinit();
    water_level_sensor_deinit();
    feed_level_sensor_deinit();
    gas_sensor_deinit();
    
    sensor_health = 0;
    initialized = false;
    
    ESP_LOGI(TAG, "Sensor manager deinitialized");
    return ESP_OK;
}
