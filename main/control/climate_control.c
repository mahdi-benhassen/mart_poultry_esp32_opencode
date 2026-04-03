#include "climate_control.h"
#include "pid_controller.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "CLIMATE_CTRL";

static climate_config_t climate_config = {0};
static pid_config_t temp_pid_config = {0};
static pid_config_t humidity_pid_config = {0};
static pid_config_t ventilation_pid_config = {0};
static pid_state_t temp_pid_state = {0};
static pid_state_t humidity_pid_state = {0};
static pid_state_t ventilation_pid_state = {0};
static bool initialized = false;

esp_err_t climate_control_init(const climate_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&climate_config, config, sizeof(climate_config_t));
    
    // Initialize temperature PID controller
    temp_pid_config.kp = PID_TEMP_KP;
    temp_pid_config.ki = PID_TEMP_KI;
    temp_pid_config.kd = PID_TEMP_KD;
    temp_pid_config.setpoint = climate_config.temp_setpoint;
    temp_pid_config.output_min = 0;
    temp_pid_config.output_max = 100;
    temp_pid_config.integral_min = -50;
    temp_pid_config.integral_max = 50;
    temp_pid_config.deadband = 0.5f;
    
    pid_controller_init(&temp_pid_config, &temp_pid_state);
    
    // Initialize humidity PID controller
    humidity_pid_config.kp = PID_HUMIDITY_KP;
    humidity_pid_config.ki = PID_HUMIDITY_KI;
    humidity_pid_config.kd = PID_HUMIDITY_KD;
    humidity_pid_config.setpoint = climate_config.humidity_setpoint;
    humidity_pid_config.output_min = 0;
    humidity_pid_config.output_max = 100;
    humidity_pid_config.integral_min = -50;
    humidity_pid_config.integral_max = 50;
    humidity_pid_config.deadband = 1.0f;
    
    pid_controller_init(&humidity_pid_config, &humidity_pid_state);
    
    // Initialize ventilation PID controller
    ventilation_pid_config.kp = PID_VENTILATION_KP;
    ventilation_pid_config.ki = PID_VENTILATION_KI;
    ventilation_pid_config.kd = PID_VENTILATION_KD;
    ventilation_pid_config.setpoint = climate_config.ammonia_limit;
    ventilation_pid_config.output_min = 0;
    ventilation_pid_config.output_max = 100;
    ventilation_pid_config.integral_min = -50;
    ventilation_pid_config.integral_max = 50;
    ventilation_pid_config.deadband = 2.0f;
    
    pid_controller_init(&ventilation_pid_config, &ventilation_pid_state);
    
    initialized = true;
    
    ESP_LOGI(TAG, "Climate control initialized: Temp=%.1f°C, Humidity=%.1f%%, NH3=%.1fppm, CO2=%.1fppm",
             climate_config.temp_setpoint, climate_config.humidity_setpoint,
             climate_config.ammonia_limit, climate_config.co2_limit);
    
    return ESP_OK;
}

esp_err_t climate_control_update(const sensor_data_t *sensor_data, 
                                 actuator_states_t *actuator_states) {
    if (!initialized || sensor_data == NULL || actuator_states == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    float temp_output = 0;
    float humidity_output = 0;
    float ventilation_output = 0;
    
    // Temperature control
    if (sensor_data->temperature > 0) {
        pid_controller_update(&temp_pid_state, &temp_pid_config, 
                            sensor_data->temperature, &temp_output);
        
        // Temperature too high - increase cooling (fans)
        if (sensor_data->temperature > climate_config.temp_setpoint + climate_config.temp_tolerance) {
            actuator_states->exhaust_fan_speed = (uint8_t)temp_output;
            actuator_states->inlet_fan_speed = (uint8_t)(temp_output * 0.8f);
            actuator_states->heater_power = 0;
        }
        // Temperature too low - increase heating
        else if (sensor_data->temperature < climate_config.temp_setpoint - climate_config.temp_tolerance) {
            actuator_states->heater_power = (uint8_t)temp_output;
            actuator_states->exhaust_fan_speed = 20; // Minimum ventilation
            actuator_states->inlet_fan_speed = 10;
        }
        // Temperature within range
        else {
            actuator_states->exhaust_fan_speed = 30; // Minimum ventilation
            actuator_states->inlet_fan_speed = 20;
            actuator_states->heater_power = 0;
        }
    }
    
    // Humidity control
    if (sensor_data->humidity > 0) {
        pid_controller_update(&humidity_pid_state, &humidity_pid_config, 
                            sensor_data->humidity, &humidity_output);
        
        // Humidity too high - increase ventilation
        if (sensor_data->humidity > climate_config.humidity_setpoint + climate_config.humidity_tolerance) {
            int exhaust = actuator_states->exhaust_fan_speed + (int)(humidity_output * 0.5f);
            int inlet = actuator_states->inlet_fan_speed + (int)(humidity_output * 0.3f);
            actuator_states->exhaust_fan_speed = (uint8_t)(exhaust > 100 ? 100 : exhaust);
            actuator_states->inlet_fan_speed = (uint8_t)(inlet > 100 ? 100 : inlet);
        }
        else if (sensor_data->humidity < climate_config.humidity_setpoint - climate_config.humidity_tolerance) {
            actuator_states->exhaust_fan_speed = (uint8_t)(actuator_states->exhaust_fan_speed * 0.7f);
            actuator_states->inlet_fan_speed = (uint8_t)(actuator_states->inlet_fan_speed * 0.5f);
        }
    }
    
    // Ammonia control - critical for poultry health
    if (sensor_data->ammonia_ppm > 0) {
        pid_controller_update(&ventilation_pid_state, &ventilation_pid_config, 
                            sensor_data->ammonia_ppm, &ventilation_output);
        
        if (sensor_data->ammonia_ppm > climate_config.ammonia_limit) {
            actuator_states->exhaust_fan_speed = 100;
            actuator_states->inlet_fan_speed = 100;
            actuator_states->ventilation_position = 100;
            actuator_states->alarm_state = true;
            ESP_LOGW(TAG, "Ammonia level critical: %.1f ppm", sensor_data->ammonia_ppm);
        }
        else if (sensor_data->ammonia_ppm > climate_config.ammonia_limit * 0.8f) {
            int exhaust = actuator_states->exhaust_fan_speed + (int)(ventilation_output * 0.7f);
            int inlet = actuator_states->inlet_fan_speed + (int)(ventilation_output * 0.5f);
            actuator_states->exhaust_fan_speed = (uint8_t)(exhaust > 100 ? 100 : exhaust);
            actuator_states->inlet_fan_speed = (uint8_t)(inlet > 100 ? 100 : inlet);
            actuator_states->ventilation_position = (uint8_t)ventilation_output;
        }
    }
    
    // CO2 control
    if (sensor_data->co2_ppm > climate_config.co2_limit) {
        int exhaust = actuator_states->exhaust_fan_speed + 30;
        int inlet = actuator_states->inlet_fan_speed + 20;
        actuator_states->exhaust_fan_speed = (uint8_t)(exhaust > 100 ? 100 : exhaust);
        actuator_states->inlet_fan_speed = (uint8_t)(inlet > 100 ? 100 : inlet);
        ESP_LOGW(TAG, "CO2 level high: %.1f ppm", sensor_data->co2_ppm);
    }
    
    // Clamp fan speeds to valid range
    if (actuator_states->exhaust_fan_speed > 100) actuator_states->exhaust_fan_speed = 100;
    if (actuator_states->inlet_fan_speed > 100) actuator_states->inlet_fan_speed = 100;
    if (actuator_states->ventilation_position > 100) actuator_states->ventilation_position = 100;
    
    // Minimum ventilation requirement (always maintain some air exchange)
    if (actuator_states->exhaust_fan_speed < 15) actuator_states->exhaust_fan_speed = 15;
    if (actuator_states->inlet_fan_speed < 10) actuator_states->inlet_fan_speed = 10;
    
    ESP_LOGD(TAG, "Climate update: T=%.1f°C, H=%.1f%%, NH3=%.1fppm, CO2=%.1fppm",
             sensor_data->temperature, sensor_data->humidity, 
             sensor_data->ammonia_ppm, sensor_data->co2_ppm);
    ESP_LOGD(TAG, "Actuators: Exhaust=%d%%, Inlet=%d%%, Heater=%d%%, Vent=%d%%",
             actuator_states->exhaust_fan_speed, actuator_states->inlet_fan_speed,
             actuator_states->heater_power, actuator_states->ventilation_position);
    
    return ESP_OK;
}

esp_err_t climate_control_set_temp_setpoint(float setpoint) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    climate_config.temp_setpoint = setpoint;
    pid_controller_set_setpoint(&temp_pid_config, setpoint);
    
    ESP_LOGI(TAG, "Temperature setpoint set to %.1f°C", setpoint);
    return ESP_OK;
}

esp_err_t climate_control_set_humidity_setpoint(float setpoint) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    climate_config.humidity_setpoint = setpoint;
    pid_controller_set_setpoint(&humidity_pid_config, setpoint);
    
    ESP_LOGI(TAG, "Humidity setpoint set to %.1f%%", setpoint);
    return ESP_OK;
}

esp_err_t climate_control_set_ammonia_limit(float limit) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    climate_config.ammonia_limit = limit;
    pid_controller_set_setpoint(&ventilation_pid_config, limit);
    
    ESP_LOGI(TAG, "Ammonia limit set to %.1f ppm", limit);
    return ESP_OK;
}

esp_err_t climate_control_set_co2_limit(float limit) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    climate_config.co2_limit = limit;
    
    ESP_LOGI(TAG, "CO2 limit set to %.1f ppm", limit);
    return ESP_OK;
}

esp_err_t climate_control_get_status(char *status, size_t max_len) {
    if (!initialized || status == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    snprintf(status, max_len, 
             "Temp: %.1f°C (SP: %.1f°C), Humidity: %.1f%% (SP: %.1f%%), "
             "NH3: %.1fppm (Limit: %.1fppm), CO2: %.1fppm (Limit: %.1fppm)",
             temp_pid_state.output, climate_config.temp_setpoint,
             humidity_pid_state.output, climate_config.humidity_setpoint,
             ventilation_pid_state.output, climate_config.ammonia_limit,
             0.0f, climate_config.co2_limit);
    
    return ESP_OK;
}

esp_err_t climate_control_reset(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    pid_controller_reset(&temp_pid_state);
    pid_controller_reset(&humidity_pid_state);
    pid_controller_reset(&ventilation_pid_state);
    
    ESP_LOGI(TAG, "Climate control reset");
    return ESP_OK;
}

esp_err_t climate_control_deinit(void) {
    initialized = false;
    ESP_LOGI(TAG, "Climate control deinitialized");
    return ESP_OK;
}
