#include "gas_sensor.h"
#include "esp_log.h"
#include "esp_adc_cal.h"
#include "esp_timer.h"
#include <string.h>
#include <math.h>

static const char *TAG = "GAS_SENSOR";

#define DEFAULT_VREF    1100        // Default reference voltage in mV
#define NO_OF_SAMPLES   64          // Multisampling

// MQ-4 sensor constants for methane detection
#define MQ4_RL_VALUE           10.0f   // Load resistance in kOhms
#define MQ4_RO_CLEAN_AIR       9.83f   // Sensor resistance in clean air
#define MQ4_RO_CLEAN_AIR_FACTOR 9.83f  // Ro in clean air / Ro in 1000ppm CH4

// MQ-4 curve for methane (log-log scale)
#define MQ4_CH4_CURVE_A        2.3f    // Curve slope
#define MQ4_CH4_CURVE_B        0.72f   // Curve intercept

// MQ-2 sensor constants for LPG/smoke detection
#define MQ2_RL_VALUE           5.0f    // Load resistance in kOhms
#define MQ2_RO_CLEAN_AIR       9.83f   // Sensor resistance in clean air

// MQ-7 sensor constants for CO detection
#define MQ7_RL_VALUE           10.0f   // Load resistance in kOhms
#define MQ7_RO_CLEAN_AIR       26.0f   // Sensor resistance in clean air

static gas_sensor_config_t sensor_config = {0};
static esp_adc_cal_characteristics_t *adc_chars = NULL;
static bool initialized = false;
static float last_ppm = 0;
static bool last_alarm = false;
static bool last_danger = false;

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
    if (adc_chars != NULL) {
        free(adc_chars);
        adc_chars = NULL;
    }
    
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

/**
 * @brief Calculate gas concentration from resistance ratio
 */
static float calculate_ppm(float rs_ro_ratio, gas_type_t gas_type) {
    float ppm = 0.0f;
    
    switch (gas_type) {
        case GAS_TYPE_METHANE:
            // MQ-4 methane curve: PPM = 10^((log10(rs_ro) - b) / a)
            if (rs_ro_ratio > 0) {
                ppm = pow(10, ((log10(rs_ro_ratio) - MQ4_CH4_CURVE_B) / MQ4_CH4_CURVE_A));
            }
            break;
            
        case GAS_TYPE_LPG:
            // MQ-2 LPG curve approximation
            if (rs_ro_ratio > 0) {
                ppm = pow(10, ((log10(rs_ro_ratio) - 0.45f) / -0.38f));
            }
            break;
            
        case GAS_TYPE_CARBON_MONOXIDE:
            // MQ-7 CO curve approximation
            if (rs_ro_ratio > 0) {
                ppm = pow(10, ((log10(rs_ro_ratio) - 0.75f) / -0.65f));
            }
            break;
            
        case GAS_TYPE_SMOKE:
            // MQ-2 smoke curve approximation
            if (rs_ro_ratio > 0) {
                ppm = pow(10, ((log10(rs_ro_ratio) - 0.55f) / -0.45f));
            }
            break;
            
        case GAS_TYPE_GENERAL:
        default:
            // Generic combustible gas approximation
            if (rs_ro_ratio > 0) {
                ppm = pow(10, ((log10(rs_ro_ratio) - 0.6f) / -0.5f));
            }
            break;
    }
    
    return ppm;
}

esp_err_t gas_sensor_init(const gas_sensor_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (initialized) {
        ESP_LOGW(TAG, "Gas sensor already initialized, re-initializing...");
        gas_sensor_deinit();
    }
    
    memcpy(&sensor_config, config, sizeof(gas_sensor_config_t));
    
    // Set defaults if not specified
    if (sensor_config.samples == 0) {
        sensor_config.samples = NO_OF_SAMPLES;
    }
    if (sensor_config.calibration_scale == 0) {
        sensor_config.calibration_scale = 1.0f;
    }
    if (sensor_config.alarm_threshold_ppm == 0) {
        // Default alarm thresholds based on gas type
        switch (sensor_config.gas_type) {
            case GAS_TYPE_METHANE:
                sensor_config.alarm_threshold_ppm = 1000.0f;  // 1000 ppm
                sensor_config.danger_threshold_ppm = 5000.0f; // 5000 ppm
                break;
            case GAS_TYPE_LPG:
                sensor_config.alarm_threshold_ppm = 1000.0f;
                sensor_config.danger_threshold_ppm = 2000.0f;
                break;
            case GAS_TYPE_CARBON_MONOXIDE:
                sensor_config.alarm_threshold_ppm = 50.0f;     // 50 ppm
                sensor_config.danger_threshold_ppm = 200.0f;   // 200 ppm
                break;
            case GAS_TYPE_SMOKE:
                sensor_config.alarm_threshold_ppm = 100.0f;
                sensor_config.danger_threshold_ppm = 500.0f;
                break;
            case GAS_TYPE_GENERAL:
            default:
                sensor_config.alarm_threshold_ppm = 1000.0f;
                sensor_config.danger_threshold_ppm = 5000.0f;
                break;
        }
    }
    
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(sensor_config.adc_channel, ADC_ATTEN_DB_11);
    
    // Check calibration
    check_efuse();
    characterize_adc(ADC_UNIT_1, sensor_config.adc_channel, ADC_ATTEN_DB_11);
    
    initialized = true;
    ESP_LOGI(TAG, "Gas sensor initialized on ADC channel %d", sensor_config.adc_channel);
    ESP_LOGI(TAG, "Gas type: %s, Alarm: %.1f ppm, Danger: %.1f ppm", 
             gas_sensor_get_type_name(sensor_config.gas_type),
             sensor_config.alarm_threshold_ppm,
             sensor_config.danger_threshold_ppm);
    
    return ESP_OK;
}

esp_err_t gas_sensor_read(gas_sensor_data_t *data) {
    if (!initialized || data == NULL) {
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
    
    // Calculate sensor resistance (Rs)
    float rs = 0.0f;
    float rl = MQ4_RL_VALUE; // Default to MQ-4
    
    // Select RL based on gas type
    switch (sensor_config.gas_type) {
        case GAS_TYPE_METHANE:
            rl = MQ4_RL_VALUE;
            break;
        case GAS_TYPE_LPG:
        case GAS_TYPE_SMOKE:
            rl = MQ2_RL_VALUE;
            break;
        case GAS_TYPE_CARBON_MONOXIDE:
            rl = MQ7_RL_VALUE;
            break;
        default:
            rl = MQ4_RL_VALUE;
            break;
    }
    
    // Rs = RL * (Vc - Vout) / Vout
    if (voltage > 0 && voltage < 3300) {
        rs = rl * (3300.0f - voltage) / voltage;
    }
    
    // Calculate Ro (sensor resistance in clean air)
    float ro = MQ4_RO_CLEAN_AIR; // Default
    switch (sensor_config.gas_type) {
        case GAS_TYPE_METHANE:
            ro = MQ4_RO_CLEAN_AIR;
            break;
        case GAS_TYPE_LPG:
        case GAS_TYPE_SMOKE:
            ro = MQ2_RO_CLEAN_AIR;
            break;
        case GAS_TYPE_CARBON_MONOXIDE:
            ro = MQ7_RO_CLEAN_AIR;
            break;
        default:
            ro = MQ4_RO_CLEAN_AIR;
            break;
    }
    
    // Calculate Rs/Ro ratio
    float rs_ro_ratio = rs / ro;
    
    // Calculate PPM
    float raw_ppm = calculate_ppm(rs_ro_ratio, sensor_config.gas_type);
    
    // Apply calibration
    data->concentration_ppm = (raw_ppm * sensor_config.calibration_scale) + sensor_config.calibration_offset;
    
    // Clamp to valid range
    if (data->concentration_ppm < 0) data->concentration_ppm = 0;
    if (data->concentration_ppm > 100000) data->concentration_ppm = 100000;
    
    // Calculate percentage of LEL (Lower Explosive Limit)
    // LEL for methane = 5% = 50000 ppm
    float lel_ppm = 50000.0f;
    switch (sensor_config.gas_type) {
        case GAS_TYPE_METHANE:
            lel_ppm = 50000.0f;  // 5% LEL
            break;
        case GAS_TYPE_LPG:
            lel_ppm = 20000.0f;  // 2% LEL
            break;
        case GAS_TYPE_CARBON_MONOXIDE:
            lel_ppm = 125000.0f; // 12.5% LEL
            break;
        default:
            lel_ppm = 50000.0f;
            break;
    }
    data->percentage = (data->concentration_ppm / lel_ppm) * 100.0f;
    
    // Check alarm and danger thresholds
    data->alarm_triggered = (data->concentration_ppm >= sensor_config.alarm_threshold_ppm);
    data->danger_triggered = (data->concentration_ppm >= sensor_config.danger_threshold_ppm);
    
    // Update last values
    last_ppm = data->concentration_ppm;
    last_alarm = data->alarm_triggered;
    last_danger = data->danger_triggered;
    
    // Set metadata
    data->timestamp = esp_timer_get_time() / 1000000;
    data->gas_type = sensor_config.gas_type;
    
    ESP_LOGD(TAG, "ADC: %d, Voltage: %dmV, Rs: %.2f kΩ, Rs/Ro: %.3f, PPM: %.2f, %.2f%% LEL",
             adc_reading, voltage, rs, rs_ro_ratio, data->concentration_ppm, data->percentage);
    
    if (data->danger_triggered) {
        ESP_LOGE(TAG, "DANGER! %s level critical: %.1f ppm (%.1f%% LEL)", 
                 gas_sensor_get_type_name(sensor_config.gas_type),
                 data->concentration_ppm, data->percentage);
    } else if (data->alarm_triggered) {
        ESP_LOGW(TAG, "ALARM! %s level high: %.1f ppm (%.1f%% LEL)", 
                 gas_sensor_get_type_name(sensor_config.gas_type),
                 data->concentration_ppm, data->percentage);
    }
    
    return ESP_OK;
}

esp_err_t gas_sensor_read_ppm(float *ppm) {
    if (!initialized || ppm == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    gas_sensor_data_t data;
    esp_err_t ret = gas_sensor_read(&data);
    if (ret == ESP_OK) {
        *ppm = data.concentration_ppm;
    }
    
    return ret;
}

esp_err_t gas_sensor_check_leak(bool *is_leak) {
    if (!initialized || is_leak == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    gas_sensor_data_t data;
    esp_err_t ret = gas_sensor_read(&data);
    if (ret == ESP_OK) {
        *is_leak = data.alarm_triggered || data.danger_triggered;
    }
    
    return ret;
}

esp_err_t gas_sensor_set_alarm_threshold(float threshold_ppm) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (threshold_ppm < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    sensor_config.alarm_threshold_ppm = threshold_ppm;
    ESP_LOGI(TAG, "Alarm threshold set to %.1f ppm", threshold_ppm);
    
    return ESP_OK;
}

esp_err_t gas_sensor_set_danger_threshold(float threshold_ppm) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (threshold_ppm < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    sensor_config.danger_threshold_ppm = threshold_ppm;
    ESP_LOGI(TAG, "Danger threshold set to %.1f ppm", threshold_ppm);
    
    return ESP_OK;
}

esp_err_t gas_sensor_calibrate(float known_ppm) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    float current_ppm = 0;
    if (gas_sensor_read_ppm(&current_ppm) != ESP_OK) {
        return ESP_FAIL;
    }
    
    // Calculate new calibration parameters
    if (current_ppm > 0) {
        sensor_config.calibration_scale = known_ppm / current_ppm;
        sensor_config.calibration_offset = 0;
    } else {
        sensor_config.calibration_offset = known_ppm;
    }
    
    ESP_LOGI(TAG, "Calibration complete: scale=%.3f, offset=%.3f", 
             sensor_config.calibration_scale, sensor_config.calibration_offset);
    
    return ESP_OK;
}

int gas_sensor_get_raw(void) {
    if (!initialized) {
        return -1;
    }
    return adc1_get_raw(sensor_config.adc_channel);
}

bool gas_sensor_is_connected(void) {
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

const char* gas_sensor_get_type_name(gas_type_t gas_type) {
    switch (gas_type) {
        case GAS_TYPE_METHANE:
            return "Methane (CH4)";
        case GAS_TYPE_LPG:
            return "LPG";
        case GAS_TYPE_CARBON_MONOXIDE:
            return "Carbon Monoxide (CO)";
        case GAS_TYPE_SMOKE:
            return "Smoke";
        case GAS_TYPE_GENERAL:
        default:
            return "Combustible Gas";
    }
}

esp_err_t gas_sensor_deinit(void) {
    if (adc_chars != NULL) {
        free(adc_chars);
        adc_chars = NULL;
    }
    
    initialized = false;
    memset(&sensor_config, 0, sizeof(sensor_config));
    last_ppm = 0;
    last_alarm = false;
    last_danger = false;
    
    ESP_LOGI(TAG, "Gas sensor deinitialized");
    return ESP_OK;
}
