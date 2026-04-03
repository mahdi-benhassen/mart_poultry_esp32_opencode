#ifndef FAN_CONTROL_H
#define FAN_CONTROL_H

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t gpio_num;
    ledc_channel_t pwm_channel;
    ledc_timer_t pwm_timer;
    uint32_t pwm_frequency;
    ledc_timer_bit_t pwm_resolution;
    bool inverted;
} fan_config_t;

typedef struct {
    fan_config_t config;
    uint8_t current_speed;
    bool initialized;
} fan_instance_t;

esp_err_t fan_control_init(fan_instance_t *inst, const fan_config_t *config);
esp_err_t fan_control_set_speed(fan_instance_t *inst, uint8_t speed);
uint8_t fan_control_get_speed(fan_instance_t *inst);
esp_err_t fan_control_on(fan_instance_t *inst);
esp_err_t fan_control_off(fan_instance_t *inst);
bool fan_control_is_running(fan_instance_t *inst);
esp_err_t fan_control_deinit(fan_instance_t *inst);

#ifdef __cplusplus
}
#endif

#endif // FAN_CONTROL_H
