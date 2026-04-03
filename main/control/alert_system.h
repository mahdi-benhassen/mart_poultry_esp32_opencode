#ifndef ALERT_SYSTEM_H
#define ALERT_SYSTEM_H

#include "esp_err.h"
#include "../include/poultry_system_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool enabled;
    bool mqtt_enabled;
    bool alarm_enabled;
    uint32_t cooldown_seconds;
} alert_config_t;

typedef struct {
    alert_type_t type;
    uint32_t timestamp;
    float value;
    float threshold;
    char message[128];
} alert_event_t;

esp_err_t alert_system_init(const alert_config_t *config);
esp_err_t alert_system_check(const sensor_data_t *sensor_data);
esp_err_t alert_system_trigger(alert_type_t type, float value, float threshold,
                               const char *message);
alert_type_t alert_system_get_active(void);
esp_err_t alert_system_clear(void);
esp_err_t alert_system_get_history(alert_event_t *history, size_t max_count,
                                   size_t *count);
esp_err_t alert_system_enable(bool enabled);
bool alert_system_is_enabled(void);
esp_err_t alert_system_set_thresholds(float temp_high, float temp_low,
                                      float humidity_high, float humidity_low,
                                      float ammonia_high, float co2_high,
                                      float water_low, float feed_low);
esp_err_t alert_system_set_gas_threshold(float gas_threshold);
esp_err_t alert_system_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // ALERT_SYSTEM_H
