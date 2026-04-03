#include "actuator_manager.h"
#include "fan_control.h"
#include "heater_control.h"
#include "feeder_control.h"
#include "water_pump.h"
#include "lighting_control.h"
#include "ventilation_control.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "ACTUATOR_MGR";

static actuator_states_t current_states = {0};
static bool initialized = false;

static fan_instance_t exhaust_fan_inst = {0};
static fan_instance_t inlet_fan_inst = {0};

static uint8_t clamp_u8(uint8_t value, uint8_t min, uint8_t max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

esp_err_t actuator_manager_init(void) {
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing actuator manager...");
    
    fan_config_t exhaust_fan_config = {
        .gpio_num = EXHAUST_FAN_GPIO,
        .pwm_channel = EXHAUST_FAN_PWM_CHANNEL,
        .pwm_timer = LEDC_TIMER_0,
        .pwm_frequency = PWM_FREQUENCY,
        .pwm_resolution = PWM_RESOLUTION,
        .inverted = false
    };
    ret = fan_control_init(&exhaust_fan_inst, &exhaust_fan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize exhaust fan");
        ESP_LOGW\(TAG, "Non-critical initialization error"\);
    } else {
        ESP_LOGI(TAG, "Exhaust fan initialized");
    }
    
    fan_config_t inlet_fan_config = {
        .gpio_num = INLET_FAN_GPIO,
        .pwm_channel = INLET_FAN_PWM_CHANNEL,
        .pwm_timer = LEDC_TIMER_0,
        .pwm_frequency = PWM_FREQUENCY,
        .pwm_resolution = PWM_RESOLUTION,
        .inverted = false
    };
    ret = fan_control_init(&inlet_fan_inst, &inlet_fan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize inlet fan");
        ESP_LOGW\(TAG, "Non-critical initialization error"\);
    } else {
        ESP_LOGI(TAG, "Inlet fan initialized");
    }
    
    heater_config_t heater_cfg = {
        .gpio_num = HEATER_GPIO,
        .pwm_channel = HEATER_PWM_CHANNEL,
        .pwm_timer = LEDC_TIMER_1,
        .pwm_frequency = PWM_FREQUENCY,
        .pwm_resolution = PWM_RESOLUTION,
        .inverted = false,
        .max_temperature = 40.0f
    };
    ret = heater_control_init(&heater_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize heater");
        ESP_LOGW\(TAG, "Non-critical initialization error"\);
    } else {
        ESP_LOGI(TAG, "Heater initialized");
    }
    
    feeder_config_t feeder_cfg = {
        .gpio_num = FEEDER_MOTOR_GPIO,
        .pwm_channel = FEEDER_PWM_CHANNEL,
        .pwm_timer = LEDC_TIMER_3,
        .pwm_frequency = PWM_FREQUENCY,
        .pwm_resolution = PWM_RESOLUTION,
        .inverted = false,
        .feed_rate_g_per_sec = 10.0f
    };
    ret = feeder_control_init(&feeder_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize feeder");
        ESP_LOGW\(TAG, "Non-critical initialization error"\);
    } else {
        ESP_LOGI(TAG, "Feeder initialized");
    }
    
    water_pump_config_t pump_cfg = {
        .gpio_num = WATER_PUMP_GPIO,
        .active_high = true,
        .flow_rate_ml_per_sec = 100.0f
    };
    ret = water_pump_init(&pump_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize water pump");
        ESP_LOGW\(TAG, "Non-critical initialization error"\);
    } else {
        ESP_LOGI(TAG, "Water pump initialized");
    }
    
    lighting_config_t lighting_cfg = {
        .gpio_num = LIGHTING_GPIO,
        .pwm_channel = LIGHTING_PWM_CHANNEL,
        .pwm_timer = LEDC_TIMER_2,
        .pwm_frequency = PWM_FREQUENCY,
        .pwm_resolution = PWM_RESOLUTION,
        .inverted = false
    };
    ret = lighting_control_init(&lighting_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize lighting");
        ESP_LOGW\(TAG, "Non-critical initialization error"\);
    } else {
        ESP_LOGI(TAG, "Lighting initialized");
    }
    
    ventilation_config_t vent_cfg = {
        .servo_gpio = VENTILATION_SERVO_GPIO,
        .pwm_channel = VENTILATION_PWM_CHANNEL,
        .pwm_timer = VENTILATION_PWM_TIMER,
        .pwm_frequency = 50,
        .pwm_resolution = LEDC_TIMER_16_BIT,
        .min_pulse_us = 500,
        .max_pulse_us = 2500
    };
    ret = ventilation_control_init(&vent_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ventilation");
        ESP_LOGW\(TAG, "Non-critical initialization error"\);
    } else {
        ESP_LOGI(TAG, "Ventilation initialized");
    }
    
    gpio_config_t alarm_conf = {
        .pin_bit_mask = (1ULL << ALARM_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret = gpio_config(&alarm_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure alarm GPIO");
        ESP_LOGW\(TAG, "Non-critical initialization error"\);
    } else {
        gpio_set_level(ALARM_GPIO, 0);
    }
    
    gpio_config_t curtain_conf = {
        .pin_bit_mask = (1ULL << CURTAIN_MOTOR_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret = gpio_config(&curtain_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure curtain GPIO");
        ESP_LOGW\(TAG, "Non-critical initialization error"\);
    } else {
        gpio_set_level(CURTAIN_MOTOR_GPIO, 0);
    }
    
    memset(&current_states, 0, sizeof(actuator_states_t));
    initialized = true;
    
    ESP_LOGI(TAG, "Actuator manager initialized successfully");
    return ESP_OK;
}

esp_err_t actuator_manager_set_states(const actuator_states_t *states) {
    if (!initialized || states == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    actuator_states_t validated_states = {
        .exhaust_fan_speed = clamp_u8(states->exhaust_fan_speed, 0, 100),
        .inlet_fan_speed = clamp_u8(states->inlet_fan_speed, 0, 100),
        .heater_power = clamp_u8(states->heater_power, 0, 100),
        .feeder_speed = clamp_u8(states->feeder_speed, 0, 100),
        .water_pump_state = clamp_u8(states->water_pump_state, 0, 1),
        .lighting_intensity = clamp_u8(states->lighting_intensity, 0, 100),
        .ventilation_position = clamp_u8(states->ventilation_position, 0, 100),
        .curtain_position = clamp_u8(states->curtain_position, 0, 100),
        .alarm_state = states->alarm_state
    };
    
    esp_err_t ret;
    
    ret = fan_control_set_speed(&exhaust_fan_inst, validated_states.exhaust_fan_speed);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set exhaust fan speed");
    }
    
    ret = fan_control_set_speed(&inlet_fan_inst, validated_states.inlet_fan_speed);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set inlet fan speed");
    }
    
    ret = heater_control_set_power(validated_states.heater_power);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set heater power");
    }
    
    ret = feeder_control_set_speed(validated_states.feeder_speed);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set feeder speed");
    }
    
    if (validated_states.water_pump_state) {
        ret = water_pump_on();
    } else {
        ret = water_pump_off();
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set water pump state");
    }
    
    ret = lighting_control_set_intensity(validated_states.lighting_intensity);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set lighting intensity");
    }
    
    ret = ventilation_control_set_position(validated_states.ventilation_position);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set ventilation position");
    }
    
    gpio_set_level(ALARM_GPIO, validated_states.alarm_state ? 1 : 0);
    
    memcpy(&current_states, &validated_states, sizeof(actuator_states_t));
    
    return ESP_OK;
}

esp_err_t actuator_manager_get_states(actuator_states_t *states) {
    if (!initialized || states == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    current_states.exhaust_fan_speed = fan_control_get_speed(&exhaust_fan_inst);
    current_states.inlet_fan_speed = fan_control_get_speed(&inlet_fan_inst);
    current_states.heater_power = heater_control_get_power();
    current_states.feeder_speed = feeder_control_get_speed();
    current_states.water_pump_state = water_pump_is_running() ? 1 : 0;
    current_states.lighting_intensity = lighting_control_get_intensity();
    current_states.ventilation_position = ventilation_control_get_position();
    current_states.alarm_state = gpio_get_level(ALARM_GPIO);
    
    memcpy(states, &current_states, sizeof(actuator_states_t));
    
    return ESP_OK;
}

esp_err_t actuator_manager_set_exhaust_fan(uint8_t speed) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t clamped_speed = clamp_u8(speed, 0, 100);
    current_states.exhaust_fan_speed = clamped_speed;
    return fan_control_set_speed(&exhaust_fan_inst, clamped_speed);
}

esp_err_t actuator_manager_set_inlet_fan(uint8_t speed) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t clamped_speed = clamp_u8(speed, 0, 100);
    current_states.inlet_fan_speed = clamped_speed;
    return fan_control_set_speed(&inlet_fan_inst, clamped_speed);
}

esp_err_t actuator_manager_set_heater(uint8_t power) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t clamped_power = clamp_u8(power, 0, 100);
    current_states.heater_power = clamped_power;
    return heater_control_set_power(clamped_power);
}

esp_err_t actuator_manager_set_feeder(uint8_t speed) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t clamped_speed = clamp_u8(speed, 0, 100);
    current_states.feeder_speed = clamped_speed;
    return feeder_control_set_speed(clamped_speed);
}

esp_err_t actuator_manager_set_water_pump(uint8_t state) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t clamped_state = clamp_u8(state, 0, 1);
    current_states.water_pump_state = clamped_state;
    if (clamped_state) {
        return water_pump_on();
    } else {
        return water_pump_off();
    }
}

esp_err_t actuator_manager_set_lighting(uint8_t intensity) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t clamped_intensity = clamp_u8(intensity, 0, 100);
    current_states.lighting_intensity = clamped_intensity;
    return lighting_control_set_intensity(clamped_intensity);
}

esp_err_t actuator_manager_set_ventilation(uint8_t position) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t clamped_position = clamp_u8(position, 0, 100);
    current_states.ventilation_position = clamped_position;
    return ventilation_control_set_position(clamped_position);
}

esp_err_t actuator_manager_set_alarm(bool state) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    current_states.alarm_state = state;
    gpio_set_level(ALARM_GPIO, state ? 1 : 0);
    return ESP_OK;
}

esp_err_t actuator_manager_emergency_stop(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGW(TAG, "Emergency stop activated!");
    
    fan_control_off(&exhaust_fan_inst);
    fan_control_off(&inlet_fan_inst);
    heater_control_off();
    feeder_control_stop();
    water_pump_off();
    lighting_control_off();
    ventilation_control_close();
    gpio_set_level(ALARM_GPIO, 1);
    
    memset(&current_states, 0, sizeof(actuator_states_t));
    current_states.alarm_state = true;
    
    return ESP_OK;
}

esp_err_t actuator_manager_deinit(void) {
    if (initialized) {
        fan_control_deinit(&exhaust_fan_inst);
        fan_control_deinit(&inlet_fan_inst);
        heater_control_deinit();
        feeder_control_deinit();
        water_pump_deinit();
        lighting_control_deinit();
        ventilation_control_deinit();
        
        initialized = false;
        memset(&current_states, 0, sizeof(actuator_states_t));
        
        ESP_LOGI(TAG, "Actuator manager deinitialized");
    }
    return ESP_OK;
}
