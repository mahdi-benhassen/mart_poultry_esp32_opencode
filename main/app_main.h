#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "esp_err.h"
#include "include/poultry_system_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_main(void);

sensor_data_t get_cached_sensor_data(void);
actuator_states_t get_cached_actuator_states(void);
system_status_t get_cached_system_status(void);
alert_type_t get_cached_active_alert(void);

#ifdef __cplusplus
}
#endif

#endif // APP_MAIN_H
