#include "water_level_sensor.h"
#include "esp_log.h"
#include "esp_adc_cal.h"
#include <string.h>

static const char *TAG = "WATER_LEVEL";

#define DEFAULT_VREF    1100        // Default reference voltage in mV
#define NO_OF_SAMPLES   64          // Multisampling

static water_level_config_t sensor_config = {0};
static esp_adc_cal_characteristics_t *adc_chars = NULL;
static bool initialized = false;

/**
 * @brief Check if ADC calibration values are available
 */
static bool check_efuse(void) {
#if CONFIG_IDF_TARGET_ESP32
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Two Point: Supported");
        return true;
    } else {
        ESP_LOGW(TAG, "eFuse Two Point: NOT supported");
    }
#elif CONFIG_IDF_TARGET_ESP32S2
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Two Point: Supported");
        return true;
    } else {
        ESP_LOGW(TAG, "eFuse Two Point: NOT supported");
    }
#endif
    return false;
}

/**
 * @brief Characterize ADC
 */
static void characterize_adc(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten) {
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    if (adc_chars == NULL) {
        ESP_LOGE(TAG, "No memory for ADC characteristics");
        return;
    }
    
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "ADC characterized using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "ADC characterized using eFuse Vref");
    } else {
        ESP_LOGI(TAG, "ADC characterized using Default Vref");
    }
}

esp_err_t water_level_sensor_init(const water_level_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&sensor_config, config, sizeof(water_level_config_t));
    
    // Set defaults if not specified
    if (sensor_config.samples == 0) {
        sensor_config.samples = NO_OF_SAMPLES;
    }
    if (sensor_config.calibration_scale == 0) {
        sensor_config.calibration_scale = 1.0f;
    }
    if (sensor_config.tank_height_cm == 0) {
        sensor_config.tank_height_cm = 100.0f; // Default 100cm tank
    }
    
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(sensor_config.adc_channel, ADC_ATTEN_DB_11);
    
    // Check calibration
    check_efuse();
    characterize_adc(ADC_UNIT_1, sensor_config.adc_channel, ADC_ATTEN_DB_11);
    
    initialized = true;
    ESP_LOGI(TAG, "Water level sensor initialized on ADC channel %d", sensor_config.adc_channel);
    
    return ESP_OK;
}

esp_err_t water_level_sensor_read(float *percent) {
    if (!initialized || percent == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Multisampling for noise reduction
    uint32_t adc_reading = 0;
    for (int i = 0; i < sensor_config.samples; i++) {
        adc_reading += adc1_get_raw(sensor_config.adc_channel);
    }
    adc_reading /= sensor_config.samples;
    
    // Convert to voltage
    uint32_t voltage = 0;
    if (adc_chars != NULL) {
        voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    } else {
        // Fallback calculation
        voltage = (adc_reading * 3300) / 4095;
    }
    
    // Convert voltage to percentage (ultrasonic/float sensor characteristic)
    // Typical: 0V = 0%, 3.3V = 100%
    float raw_percent = (voltage / 3300.0f) * 100.0f;
    
    // Apply calibration
    *percent = (raw_percent * sensor_config.calibration_scale) + sensor_config.calibration_offset;
    
    // Clamp to valid range
    if (*percent < 0) *percent = 0;
    if (*percent > 100) *percent = 100;
    
    ESP_LOGD(TAG, "ADC: %d, Voltage: %dmV, Level: %.1f%%", adc_reading, voltage, *percent);
    
    return ESP_OK;
}

esp_err_t water_level_sensor_read_cm(float *cm) {
    if (!initialized || cm == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    float percent = 0;
    esp_err_t ret = water_level_sensor_read(&percent);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Convert percentage to centimeters
    *cm = (percent / 100.0f) * sensor_config.tank_height_cm;
    
    return ESP_OK;
}

esp_err_t water_level_sensor_calibrate(float known_percent) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    float current_percent = 0;
    if (water_level_sensor_read(&current_percent) != ESP_OK) {
        return ESP_FAIL;
    }
    
    // Calculate new calibration parameters
    if (current_percent > 0) {
        sensor_config.calibration_scale = known_percent / current_percent;
        sensor_config.calibration_offset = 0;
    } else {
        sensor_config.calibration_offset = known_percent;
    }
    
    ESP_LOGI(TAG, "Calibration complete: scale=%.3f, offset=%.3f", 
             sensor_config.calibration_scale, sensor_config.calibration_offset);
    
    return ESP_OK;
}

int water_level_sensor_get_raw(void) {
    if (!initialized) {
        return -1;
    }
    return adc1_get_raw(sensor_config.adc_channel);
}

bool water_level_sensor_is_connected(void) {
    if (!initialized) {
        return false;
    }
    
    // Check if sensor is responding by reading raw value
    int raw = adc1_get_raw(sensor_config.adc_channel);
    
    // If reading is at max or min, sensor might be disconnected
    if (raw <= 10 || raw >= 4090) {
        return false;
    }
    
    return true;
}

esp_err_t water_level_sensor_deinit(void) {
    if (adc_chars != NULL) {
        free(adc_chars);
        adc_chars = NULL;
    }
    
    initialized = false;
    memset(&sensor_config, 0, sizeof(sensor_config));
    
    ESP_LOGI(TAG, "Water level sensor deinitialized");
    return ESP_OK;
}
