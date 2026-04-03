#include "app_main.h"
#include "include/poultry_system_config.h"
#include "sensors/sensor_manager.h"
#include "sensors/gas_sensor.h"
#include "actuators/actuator_manager.h"
#include "actuators/buzzer_control.h"
#include "control/climate_control.h"
#include "control/feeding_schedule.h"
#include "control/lighting_schedule.h"
#include "control/alert_system.h"
#include "control/gas_leak_detector.h"
#include "connectivity/wifi_manager.h"
#include "connectivity/mqtt_client.h"
#include "connectivity/web_server.h"
#include "data/data_logger.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <string.h>
#include <time.h>

static const char *TAG = "APP_MAIN";

#define NVS_NAMESPACE "poultry_cfg"

TaskHandle_t sensor_task_handle = NULL;
TaskHandle_t control_task_handle = NULL;
TaskHandle_t mqtt_task_handle = NULL;
TaskHandle_t web_server_task_handle = NULL;
TaskHandle_t data_logger_task_handle = NULL;

QueueHandle_t sensor_data_queue = NULL;
QueueHandle_t actuator_queue = NULL;

static SemaphoreHandle_t system_status_mutex = NULL;

system_status_t system_status = {0};
system_config_t system_config = {0};
static bool system_initialized = false;

extern sensor_data_t get_cached_sensor_data(void);
extern actuator_states_t get_cached_actuator_states(void);
extern system_status_t get_cached_system_status(void);
extern alert_type_t get_cached_active_alert(void);

sensor_data_t get_cached_sensor_data(void) {
    sensor_data_t data = {0};
    if (system_status_mutex != NULL && xSemaphoreTake(system_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(&data, &system_status.current_sensor_data, sizeof(sensor_data_t));
        xSemaphoreGive(system_status_mutex);
    }
    return data;
}

actuator_states_t get_cached_actuator_states(void) {
    actuator_states_t states = {0};
    if (system_status_mutex != NULL && xSemaphoreTake(system_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(&states, &system_status.actuator_states, sizeof(actuator_states_t));
        xSemaphoreGive(system_status_mutex);
    }
    return states;
}

system_status_t get_cached_system_status(void) {
    system_status_t status = {0};
    if (system_status_mutex != NULL && xSemaphoreTake(system_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(&status, &system_status, sizeof(system_status_t));
        xSemaphoreGive(system_status_mutex);
    }
    return status;
}

alert_type_t get_cached_active_alert(void) {
    alert_type_t alert = ALERT_NONE;
    if (system_status_mutex != NULL && xSemaphoreTake(system_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        alert = system_status.active_alert;
        xSemaphoreGive(system_status_mutex);
    }
    return alert;
}

static esp_err_t load_config_from_nvs(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed (%s), using defaults", esp_err_to_name(err));
        return err;
    }
    
    size_t ssid_len = sizeof(system_config.wifi_ssid);
    size_t pass_len = sizeof(system_config.wifi_password);
    size_t broker_len = sizeof(system_config.mqtt_broker);
    size_t username_len = sizeof(system_config.mqtt_username);
    size_t password_len = sizeof(system_config.mqtt_password);
    size_t client_id_len = sizeof(system_config.mqtt_client_id);
    
    nvs_get_str(nvs_handle, "wifi_ssid", system_config.wifi_ssid, &ssid_len);
    nvs_get_str(nvs_handle, "wifi_pass", system_config.wifi_password, &pass_len);
    nvs_get_str(nvs_handle, "mqtt_broker", system_config.mqtt_broker, &broker_len);
    nvs_get_u16(nvs_handle, "mqtt_port", &system_config.mqtt_port);
    nvs_get_str(nvs_handle, "mqtt_username", system_config.mqtt_username, &username_len);
    nvs_get_str(nvs_handle, "mqtt_password", system_config.mqtt_password, &password_len);
    nvs_get_str(nvs_handle, "mqtt_client_id", system_config.mqtt_client_id, &client_id_len);
    uint32_t temp_raw = 0, humidity_raw = 0;
    size_t temp_size = sizeof(temp_raw), humidity_size = sizeof(humidity_raw);
    esp_err_t temp_ret = nvs_get_u32(nvs_handle, "temp_setpoint_raw", &temp_raw);
    esp_err_t hum_ret = nvs_get_u32(nvs_handle, "humidity_setpoint_raw", &humidity_raw);
    if (temp_ret == ESP_OK && temp_raw != 0) {
        memcpy(&system_config.temp_setpoint, &temp_raw, sizeof(float));
    }
    if (hum_ret == ESP_OK && humidity_raw != 0) {
        memcpy(&system_config.humidity_setpoint, &humidity_raw, sizeof(float));
    }
    
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Configuration loaded from NVS");
    return ESP_OK;
}

static esp_err_t save_config_to_nvs(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed (%s)", esp_err_to_name(err));
        return err;
    }
    
    nvs_set_str(nvs_handle, "wifi_ssid", system_config.wifi_ssid);
    nvs_set_str(nvs_handle, "wifi_pass", system_config.wifi_password);
    nvs_set_str(nvs_handle, "mqtt_broker", system_config.mqtt_broker);
    nvs_set_u16(nvs_handle, "mqtt_port", system_config.mqtt_port);
    nvs_set_str(nvs_handle, "mqtt_username", system_config.mqtt_username);
    nvs_set_str(nvs_handle, "mqtt_password", system_config.mqtt_password);
    nvs_set_str(nvs_handle, "mqtt_client_id", system_config.mqtt_client_id);
    uint32_t temp_raw, humidity_raw;
    memcpy(&temp_raw, &system_config.temp_setpoint, sizeof(float));
    memcpy(&humidity_raw, &system_config.humidity_setpoint, sizeof(float));
    nvs_set_u32(nvs_handle, "temp_setpoint_raw", temp_raw);
    nvs_set_u32(nvs_handle, "humidity_setpoint_raw", humidity_raw);
    
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Configuration saved to NVS");
    }
    
    return err;
}

static void sensor_task(void *pvParameters) {
    ESP_LOGI(TAG, "Sensor task started");
    
    sensor_data_t sensor_data;
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        if (sensor_manager_read_all(&sensor_data) == ESP_OK) {
            xQueueOverwrite(sensor_data_queue, &sensor_data);
            
            if (system_status_mutex != NULL && xSemaphoreTake(system_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                memcpy(&system_status.current_sensor_data, &sensor_data, sizeof(sensor_data_t));
                xSemaphoreGive(system_status_mutex);
            }
            
            ESP_LOGD(TAG, "Sensor data: T=%.1f°C, H=%.1f%%, NH3=%.1fppm, CO2=%.1fppm",
                     sensor_data.temperature, sensor_data.humidity,
                     sensor_data.ammonia_ppm, sensor_data.co2_ppm);
        } else {
            ESP_LOGW(TAG, "Failed to read sensors");
        }
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synchronized: %s", asctime(localtime(&tv->tv_sec)));
}

static void init_sntp(void) {
    setenv("TZ", "UTC0", 1);
    tzset();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
    
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2024 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time (attempt %d/%d)...", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    
    if (timeinfo.tm_year >= (2024 - 1900)) {
        ESP_LOGI(TAG, "System time synchronized");
    } else {
        ESP_LOGW(TAG, "Failed to sync time, using boot time");
    }
}

static void control_task(void *pvParameters) {
    ESP_LOGI(TAG, "Control task started");
    
    sensor_data_t sensor_data;
    actuator_states_t actuator_states;
    TickType_t last_wake_time = xTaskGetTickCount();
    uint8_t last_feed_hour = 255;
    uint8_t last_light_hour = 255;
    
    while (1) {
        if (xQueueReceive(sensor_data_queue, &sensor_data, pdMS_TO_TICKS(1000)) == pdTRUE) {
            if (system_config.operating_mode == MODE_MAINTENANCE) {
                actuator_manager_emergency_stop();
                vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(CONTROL_LOOP_INTERVAL_MS));
                continue;
            }
            
            climate_control_update(&sensor_data, &actuator_states);
            
            time_t now;
            time(&now);
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            uint8_t current_hour = timeinfo.tm_hour;
            uint8_t current_minute = timeinfo.tm_min;
            
            if (system_config.operating_mode != MODE_MANUAL) {
                uint8_t feed_amount = 0;
                if (feeding_schedule_is_due(current_hour, current_minute, &feed_amount) && current_hour != last_feed_hour) {
                    ESP_LOGI(TAG, "Feeding due: %d grams", feed_amount);
                    feeding_schedule_execute(feed_amount);
                    last_feed_hour = current_hour;
                }
                
                uint8_t light_intensity = 0;
                if (lighting_schedule_is_due(current_hour, current_minute, &light_intensity) && current_hour != last_light_hour) {
                    ESP_LOGI(TAG, "Lighting change due: %d%%", light_intensity);
                    lighting_schedule_execute(light_intensity);
                    last_light_hour = current_hour;
                }
            }
            
            alert_system_check(&sensor_data);
            
            if (system_config.operating_mode != MODE_MANUAL) {
                gas_leak_event_t gas_event;
                gas_leak_detector_check(&gas_event);
            }
            
            actuator_manager_set_states(&actuator_states);
            
            if (system_status_mutex != NULL && xSemaphoreTake(system_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                memcpy(&system_status.actuator_states, &actuator_states, sizeof(actuator_states_t));
                system_status.active_alert = alert_system_get_active();
                xSemaphoreGive(system_status_mutex);
            }
            
            xQueueOverwrite(actuator_queue, &actuator_states);
        }
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(CONTROL_LOOP_INTERVAL_MS));
    }
}

static void mqtt_task(void *pvParameters) {
    ESP_LOGI(TAG, "MQTT task started");
    
    sensor_data_t sensor_data;
    actuator_states_t actuator_states;
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        if (mqtt_client_is_connected()) {
            if (xQueuePeek(sensor_data_queue, &sensor_data, pdMS_TO_TICKS(100)) == pdTRUE) {
                mqtt_client_publish_sensor_data(&sensor_data);
            }
            
            if (xQueuePeek(actuator_queue, &actuator_states, pdMS_TO_TICKS(100)) == pdTRUE) {
                mqtt_client_publish_actuator_states(&actuator_states);
            }
            
            alert_type_t active_alert = alert_system_get_active();
            if (active_alert != ALERT_NONE) {
                char alert_msg[128];
                snprintf(alert_msg, sizeof(alert_msg), "Alert type: %d", active_alert);
                mqtt_client_publish_alert(active_alert, alert_msg);
            }
        }
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(MQTT_PUBLISH_INTERVAL_MS));
    }
}

static void web_server_task(void *pvParameters) {
    ESP_LOGI(TAG, "Web server task started");
    
    web_server_config_t web_config = {
        .port = WEB_SERVER_PORT,
        .max_connections = WEB_MAX_CONNECTIONS,
        .stack_size = WEB_STACK_SIZE
    };
    
    if (web_server_init(&web_config) == ESP_OK) {
        web_server_start();
        ESP_LOGI(TAG, "Web server started on port %d", WEB_SERVER_PORT);
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

static void data_logger_task(void *pvParameters) {
    ESP_LOGI(TAG, "Data logger task started");
    
    sensor_data_t sensor_data;
    actuator_states_t actuator_states;
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        if (data_logger_is_enabled()) {
            if (xQueuePeek(sensor_data_queue, &sensor_data, pdMS_TO_TICKS(1000)) == pdTRUE &&
                xQueuePeek(actuator_queue, &actuator_states, pdMS_TO_TICKS(1000)) == pdTRUE) {
                
                alert_type_t active_alert = ALERT_NONE;
                if (system_status_mutex != NULL && xSemaphoreTake(system_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    active_alert = system_status.active_alert;
                    xSemaphoreGive(system_status_mutex);
                }
                
                data_logger_log(&sensor_data, &actuator_states, active_alert);
            }
        }
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(DATA_LOG_INTERVAL_MS));
    }
}

static void init_system_config(void) {
    strncpy(system_config.wifi_ssid, "YOUR_WIFI_SSID", sizeof(system_config.wifi_ssid) - 1);
    system_config.wifi_ssid[sizeof(system_config.wifi_ssid) - 1] = '\0';
    
    strncpy(system_config.wifi_password, "YOUR_WIFI_PASSWORD", sizeof(system_config.wifi_password) - 1);
    system_config.wifi_password[sizeof(system_config.wifi_password) - 1] = '\0';
    
    strncpy(system_config.mqtt_broker, "mqtt://broker.hivemq.com", sizeof(system_config.mqtt_broker) - 1);
    system_config.mqtt_broker[sizeof(system_config.mqtt_broker) - 1] = '\0';
    system_config.mqtt_port = 1883;
    
    strncpy(system_config.mqtt_username, "", sizeof(system_config.mqtt_username) - 1);
    system_config.mqtt_username[sizeof(system_config.mqtt_username) - 1] = '\0';
    
    strncpy(system_config.mqtt_password, "", sizeof(system_config.mqtt_password) - 1);
    system_config.mqtt_password[sizeof(system_config.mqtt_password) - 1] = '\0';
    
    strncpy(system_config.mqtt_client_id, "poultry_esp32", sizeof(system_config.mqtt_client_id) - 1);
    system_config.mqtt_client_id[sizeof(system_config.mqtt_client_id) - 1] = '\0';
    
    system_config.temp_setpoint = 26.0f;
    system_config.humidity_setpoint = 60.0f;
    system_config.ammonia_limit = 25.0f;
    system_config.co2_limit = 3000.0f;
    
    system_config.operating_mode = MODE_AUTOMATIC;
    system_config.alerts_enabled = true;
    system_config.data_logging_enabled = true;
    system_config.mqtt_enabled = true;
    
    memset(system_config.feeding_schedule, 0, sizeof(system_config.feeding_schedule));
    system_config.feeding_schedule[6] = 50;
    system_config.feeding_schedule[7] = 60;
    system_config.feeding_schedule[8] = 50;
    system_config.feeding_schedule[11] = 40;
    system_config.feeding_schedule[12] = 50;
    system_config.feeding_schedule[13] = 40;
    system_config.feeding_schedule[15] = 40;
    system_config.feeding_schedule[16] = 50;
    system_config.feeding_schedule[17] = 40;
    system_config.feeding_schedule[18] = 30;
    system_config.feeding_schedule[19] = 20;
    
    memset(system_config.lighting_schedule, 0, sizeof(system_config.lighting_schedule));
    for (int i = 6; i <= 18; i++) {
        system_config.lighting_schedule[i] = 100;
    }
    system_config.lighting_schedule[5] = 20;
    system_config.lighting_schedule[19] = 80;
    system_config.lighting_schedule[20] = 60;
    system_config.lighting_schedule[21] = 40;
    system_config.lighting_schedule[22] = 20;
    
    system_config.config_version = 1;
    
    load_config_from_nvs();
}

static esp_err_t init_subsystems(void) {
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing subsystems...");
    
    ret = sensor_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize sensor manager");
        return ret;
    }
    ESP_LOGI(TAG, "Sensor manager initialized");
    
    ret = actuator_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize actuator manager");
        return ret;
    }
    ESP_LOGI(TAG, "Actuator manager initialized");
    
    buzzer_config_t buzzer_cfg = {
        .gpio_num = ALARM_GPIO,
        .active_high = true,
        .frequency_hz = 2000,
        .duty_cycle = 50
    };
    ret = buzzer_control_init(&buzzer_cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize buzzer (non-critical)");
    }
    
    climate_config_t climate_cfg = {
        .temp_setpoint = system_config.temp_setpoint,
        .humidity_setpoint = system_config.humidity_setpoint,
        .ammonia_limit = system_config.ammonia_limit,
        .co2_limit = system_config.co2_limit,
        .temp_tolerance = 2.0f,
        .humidity_tolerance = 5.0f
    };
    ret = climate_control_init(&climate_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize climate control");
        return ret;
    }
    ESP_LOGI(TAG, "Climate control initialized");
    
    ret = feeding_schedule_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize feeding schedule");
        return ret;
    }
    feeding_schedule_set(system_config.feeding_schedule);
    ESP_LOGI(TAG, "Feeding schedule initialized");
    
    ret = lighting_schedule_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize lighting schedule");
        return ret;
    }
    lighting_schedule_set(system_config.lighting_schedule);
    ESP_LOGI(TAG, "Lighting schedule initialized");
    
    alert_config_t alert_cfg = {
        .enabled = system_config.alerts_enabled,
        .mqtt_enabled = system_config.mqtt_enabled,
        .alarm_enabled = true,
        .cooldown_seconds = 300
    };
    ret = alert_system_init(&alert_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize alert system");
        return ret;
    }
    ESP_LOGI(TAG, "Alert system initialized");
    
    gas_leak_config_t gas_leak_cfg = {
        .enabled = true,
        .alarm_threshold = GAS_LEAK_THRESHOLD_PPM,
        .danger_threshold = GAS_DANGER_THRESHOLD_PPM,
        .buzzer_enabled = true,
        .ventilation_enabled = true,
        .ventilation_speed = 100,
        .cooldown_seconds = 60
    };
    ret = gas_leak_detector_init(&gas_leak_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize gas leak detector");
        return ret;
    }
    ESP_LOGI(TAG, "Gas leak detector initialized");
    
    poultry_wifi_config_t wifi_cfg = {
        .ssid = "",
        .password = "",
        .max_retry = WIFI_RETRY_MAX,
        .timeout_ms = WIFI_CONNECT_TIMEOUT_MS
    };
    strncpy(wifi_cfg.ssid, system_config.wifi_ssid, sizeof(wifi_cfg.ssid) - 1);
    wifi_cfg.ssid[sizeof(wifi_cfg.ssid) - 1] = '\0';
    strncpy(wifi_cfg.password, system_config.wifi_password, sizeof(wifi_cfg.password) - 1);
    wifi_cfg.password[sizeof(wifi_cfg.password) - 1] = '\0';
    
    ret = wifi_manager_init(&wifi_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi manager");
        return ret;
    }
    ESP_LOGI(TAG, "WiFi manager initialized");
    
    if (system_config.mqtt_enabled) {
        mqtt_config_t mqtt_cfg = {
            .broker = "",
            .port = system_config.mqtt_port,
            .username = "",
            .password = "",
            .client_id = "",
            .keepalive = MQTT_KEEPALIVE,
            .qos = MQTT_QOS,
            .clean_session = true
        };
        strncpy(mqtt_cfg.broker, system_config.mqtt_broker, sizeof(mqtt_cfg.broker) - 1);
        mqtt_cfg.broker[sizeof(mqtt_cfg.broker) - 1] = '\0';
        strncpy(mqtt_cfg.username, system_config.mqtt_username, sizeof(mqtt_cfg.username) - 1);
        mqtt_cfg.username[sizeof(mqtt_cfg.username) - 1] = '\0';
        strncpy(mqtt_cfg.password, system_config.mqtt_password, sizeof(mqtt_cfg.password) - 1);
        mqtt_cfg.password[sizeof(mqtt_cfg.password) - 1] = '\0';
        strncpy(mqtt_cfg.client_id, system_config.mqtt_client_id, sizeof(mqtt_cfg.client_id) - 1);
        mqtt_cfg.client_id[sizeof(mqtt_cfg.client_id) - 1] = '\0';
        
        ret = mqtt_client_init(&mqtt_cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize MQTT client");
            return ret;
        }
        ESP_LOGI(TAG, "MQTT client initialized");
    }
    
    data_logger_config_t logger_cfg = {
        .enabled = system_config.data_logging_enabled,
        .log_interval_ms = DATA_LOG_INTERVAL_MS,
        .max_file_size = LOG_FILE_MAX_SIZE,
        .max_files = MAX_LOG_FILES
    };
    ret = data_logger_init(&logger_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize data logger");
        return ret;
    }
    ESP_LOGI(TAG, "Data logger initialized");
    
    ESP_LOGI(TAG, "All subsystems initialized successfully");
    return ESP_OK;
}

static esp_err_t create_tasks(void) {
    BaseType_t ret;
    
    ESP_LOGI(TAG, "Creating tasks...");
    
    sensor_data_queue = xQueueCreate(1, sizeof(sensor_data_t));
    actuator_queue = xQueueCreate(1, sizeof(actuator_states_t));
    
    if (sensor_data_queue == NULL || actuator_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return ESP_FAIL;
    }
    
    system_status_mutex = xSemaphoreCreateMutex();
    if (system_status_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create system status mutex");
        return ESP_FAIL;
    }
    
    ret = xTaskCreatePinnedToCore(
        sensor_task,
        "sensor_task",
        SENSOR_TASK_STACK_SIZE,
        NULL,
        SENSOR_TASK_PRIORITY,
        &sensor_task_handle,
        0
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor task: %d", ret);
        return ESP_FAIL;
    }
    
    ret = xTaskCreatePinnedToCore(
        control_task,
        "control_task",
        CONTROL_TASK_STACK_SIZE,
        NULL,
        CONTROL_TASK_PRIORITY,
        &control_task_handle,
        1
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create control task: %d", ret);
        return ESP_FAIL;
    }
    
    if (system_config.mqtt_enabled) {
        ret = xTaskCreatePinnedToCore(
            mqtt_task,
            "mqtt_task",
            MQTT_TASK_STACK_SIZE,
            NULL,
            MQTT_TASK_PRIORITY,
            &mqtt_task_handle,
            0
        );
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create MQTT task: %d", ret);
            return ESP_FAIL;
        }
    }
    
    ret = xTaskCreatePinnedToCore(
        web_server_task,
        "web_server_task",
        WEB_SERVER_TASK_STACK_SIZE,
        NULL,
        WEB_SERVER_TASK_PRIORITY,
        &web_server_task_handle,
        1
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create web server task: %d", ret);
        return ESP_FAIL;
    }
    
    ret = xTaskCreatePinnedToCore(
        data_logger_task,
        "data_logger_task",
        DATA_LOGGER_TASK_STACK_SIZE,
        NULL,
        DATA_LOGGER_TASK_PRIORITY,
        &data_logger_task_handle,
        0
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create data logger task: %d", ret);
        return ESP_FAIL;
    }
    
    ret = esp_task_wdt_add(sensor_task_handle);
    if (ret == ESP_OK) {
        esp_task_wdt_add(control_task_handle);
        if (mqtt_task_handle) esp_task_wdt_add(mqtt_task_handle);
        esp_task_wdt_add(web_server_task_handle);
        esp_task_wdt_add(data_logger_task_handle);
        ESP_LOGI(TAG, "Watchdog timer enabled");
    }
    
    ESP_LOGI(TAG, "All tasks created successfully");
    return ESP_OK;
}

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Smart Poultry System v%s", SYSTEM_VERSION);
    ESP_LOGI(TAG, "========================================");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    init_system_config();
    
    if (init_subsystems() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize subsystems");
        return;
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi...");
    if (wifi_manager_connect() == ESP_OK) {
        ESP_LOGI(TAG, "WiFi connected successfully");
        init_sntp();
        
        if (system_config.mqtt_enabled) {
            ESP_LOGI(TAG, "Connecting to MQTT broker...");
            if (mqtt_client_connect() == ESP_OK) {
                ESP_LOGI(TAG, "MQTT connected successfully");
            } else {
                ESP_LOGW(TAG, "Failed to connect to MQTT broker");
            }
        }
    } else {
        ESP_LOGW(TAG, "Failed to connect to WiFi, continuing in offline mode");
    }
    
    if (create_tasks() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create tasks");
        return;
    }
    
    system_status.state = SYSTEM_STATE_NORMAL;
    system_status.uptime_seconds = 0;
    system_status.free_heap_size = esp_get_free_heap_size();
    
    system_initialized = true;
    
    save_config_to_nvs();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "System initialized successfully!");
    ESP_LOGI(TAG, "Web server: http://<ESP32_IP>:%d", WEB_SERVER_PORT);
    ESP_LOGI(TAG, "========================================");
    
    while (1) {
        if (system_status_mutex != NULL && xSemaphoreTake(system_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            system_status.uptime_seconds = esp_timer_get_time() / 1000000;
            system_status.free_heap_size = esp_get_free_heap_size();
            xSemaphoreGive(system_status_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
