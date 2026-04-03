#include "web_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "WEB_SERVER";

static httpd_handle_t server_handle = NULL;
static web_server_config_t server_config = {0};
static bool server_running = false;

static SemaphoreHandle_t data_mutex = NULL;

static sensor_data_t cached_sensor_data = {0};
static actuator_states_t cached_actuator_states = {0};
static system_status_t cached_system_status = {0};
static alert_type_t cached_active_alert = ALERT_NONE;

extern sensor_data_t get_cached_sensor_data(void);
extern actuator_states_t get_cached_actuator_states(void);
extern system_status_t get_cached_system_status(void);
extern alert_type_t get_cached_active_alert(void);

static esp_err_t update_cached_data(void) {
    if (data_mutex != NULL && xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        sensor_data_t *sensor = get_cached_sensor_data();
        actuator_states_t *actuator = get_cached_actuator_states();
        system_status_t *status = get_cached_system_status();
        
        if (sensor != NULL) memcpy(&cached_sensor_data, sensor, sizeof(sensor_data_t));
        if (actuator != NULL) memcpy(&cached_actuator_states, actuator, sizeof(actuator_states_t));
        if (status != NULL) memcpy(&cached_system_status, status, sizeof(system_status_t));
        
        cached_active_alert = get_cached_active_alert();
        
        xSemaphoreGive(data_mutex);
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t root_get_handler(httpd_req_t *req) {
    const char *html = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>Smart Poultry System</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }\n"
        "        .container { max-width: 1200px; margin: 0 auto; }\n"
        "        h1 { color: #2c3e50; text-align: center; }\n"
        "        .card { background: white; border-radius: 8px; padding: 20px; margin-bottom: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n"
        "        .card h2 { margin-top: 0; color: #34495e; border-bottom: 2px solid #3498db; padding-bottom: 10px; }\n"
        "        .sensor-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }\n"
        "        .sensor-item { background: #ecf0f1; padding: 15px; border-radius: 5px; text-align: center; }\n"
        "        .sensor-label { font-size: 14px; color: #7f8c8d; }\n"
        "        .sensor-value { font-size: 24px; font-weight: bold; color: #2c3e50; }\n"
        "        .sensor-unit { font-size: 12px; color: #95a5a6; }\n"
        "        .actuator-item { display: flex; justify-content: space-between; align-items: center; padding: 10px; background: #ecf0f1; margin: 5px 0; border-radius: 5px; }\n"
        "        .actuator-name { font-weight: bold; }\n"
        "        .actuator-value { background: #3498db; color: white; padding: 5px 15px; border-radius: 15px; }\n"
        "        .status-indicator { display: inline-block; width: 12px; height: 12px; border-radius: 50%; margin-right: 8px; }\n"
        "        .status-normal { background: #27ae60; }\n"
        "        .status-warning { background: #f39c12; }\n"
        "        .status-critical { background: #e74c3c; }\n"
        "        .alert-banner { background: #e74c3c; color: white; padding: 15px; border-radius: 5px; margin-bottom: 20px; display: none; }\n"
        "        .alert-banner.active { display: block; }\n"
        "        .btn { background: #3498db; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; margin: 5px; }\n"
        "        .btn-danger { background: #e74c3c; }\n"
        "        .btn:hover { opacity: 0.9; }\n"
        "        .system-info { display: grid; grid-template-columns: repeat(3, 1fr); gap: 15px; text-align: center; }\n"
        "        .info-item { background: #ecf0f1; padding: 15px; border-radius: 5px; }\n"
        "        .info-label { font-size: 12px; color: #7f8c8d; }\n"
        "        .info-value { font-size: 18px; font-weight: bold; color: #2c3e50; }\n"
        "        .refresh-info { text-align: center; color: #7f8c8d; font-size: 12px; margin-top: 10px; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <h1>🐔 Smart Poultry System</h1>\n"
        "        \n"
        "        <div id=\"alertBanner\" class=\"alert-banner\">\n"
        "            <strong>⚠️ ALERT: </strong><span id=\"alertMessage\"></span>\n"
        "        </div>\n"
        "        \n"
        "        <div class=\"card\">\n"
        "            <h2>📊 System Status</h2>\n"
        "            <div class=\"system-info\">\n"
        "                <div class=\"info-item\">\n"
        "                    <div class=\"info-label\">Status</div>\n"
        "                    <div class=\"info-value\"><span id=\"statusIndicator\" class=\"status-indicator status-normal\"></span><span id=\"statusText\">Normal</span></div>\n"
        "                </div>\n"
        "                <div class=\"info-item\">\n"
        "                    <div class=\"info-label\">Uptime</div>\n"
        "                    <div class=\"info-value\" id=\"uptime\">0h</div>\n"
        "                </div>\n"
        "                <div class=\"info-item\">\n"
        "                    <div class=\"info-label\">Free Heap</div>\n"
        "                    <div class=\"info-value\" id=\"freeHeap\">0 KB</div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "        \n"
        "        <div class=\"card\">\n"
        "            <h2>🌡️ Sensor Data</h2>\n"
        "            <div class=\"sensor-grid\">\n"
        "                <div class=\"sensor-item\">\n"
        "                    <div class=\"sensor-label\">Temperature</div>\n"
        "                    <div class=\"sensor-value\" id=\"temperature\">--</div>\n"
        "                    <div class=\"sensor-unit\">°C</div>\n"
        "                </div>\n"
        "                <div class=\"sensor-item\">\n"
        "                    <div class=\"sensor-label\">Humidity</div>\n"
        "                    <div class=\"sensor-value\" id=\"humidity\">--</div>\n"
        "                    <div class=\"sensor-unit\">%</div>\n"
        "                </div>\n"
        "                <div class=\"sensor-item\">\n"
        "                    <div class=\"sensor-label\">Ammonia</div>\n"
        "                    <div class=\"sensor-value\" id=\"ammonia\">--</div>\n"
        "                    <div class=\"sensor-unit\">ppm</div>\n"
        "                </div>\n"
        "                <div class=\"sensor-item\">\n"
        "                    <div class=\"sensor-label\">CO2</div>\n"
        "                    <div class=\"sensor-value\" id=\"co2\">--</div>\n"
        "                    <div class=\"sensor-unit\">ppm</div>\n"
        "                </div>\n"
        "                <div class=\"sensor-item\">\n"
        "                    <div class=\"sensor-label\">Light</div>\n"
        "                    <div class=\"sensor-value\" id=\"light\">--</div>\n"
        "                    <div class=\"sensor-unit\">lux</div>\n"
        "                </div>\n"
        "                <div class=\"sensor-item\">\n"
        "                    <div class=\"sensor-label\">Water Level</div>\n"
        "                    <div class=\"sensor-value\" id=\"waterLevel\">--</div>\n"
        "                    <div class=\"sensor-unit\">%</div>\n"
        "                </div>\n"
        "                <div class=\"sensor-item\">\n"
        "                    <div class=\"sensor-label\">Feed Level</div>\n"
        "                    <div class=\"sensor-value\" id=\"feedLevel\">--</div>\n"
        "                    <div class=\"sensor-unit\">%</div>\n"
        "                </div>\n"
        "                <div class=\"sensor-item\">\n"
        "                    <div class=\"sensor-label\">Gas</div>\n"
        "                    <div class=\"sensor-value\" id=\"gas\">--</div>\n"
        "                    <div class=\"sensor-unit\">ppm</div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "        \n"
        "        <div class=\"card\">\n"
        "            <h2>⚙️ Actuator States</h2>\n"
        "            <div id=\"actuatorList\">\n"
        "                <div class=\"actuator-item\">\n"
        "                    <span class=\"actuator-name\">Exhaust Fan</span>\n"
        "                    <span class=\"actuator-value\" id=\"exhaustFan\">0%</span>\n"
        "                </div>\n"
        "                <div class=\"actuator-item\">\n"
        "                    <span class=\"actuator-name\">Inlet Fan</span>\n"
        "                    <span class=\"actuator-value\" id=\"inletFan\">0%</span>\n"
        "                </div>\n"
        "                <div class=\"actuator-item\">\n"
        "                    <span class=\"actuator-name\">Heater</span>\n"
        "                    <span class=\"actuator-value\" id=\"heater\">0%</span>\n"
        "                </div>\n"
        "                <div class=\"actuator-item\">\n"
        "                    <span class=\"actuator-name\">Feeder</span>\n"
        "                    <span class=\"actuator-value\" id=\"feeder\">0%</span>\n"
        "                </div>\n"
        "                <div class=\"actuator-item\">\n"
        "                    <span class=\"actuator-name\">Water Pump</span>\n"
        "                    <span class=\"actuator-value\" id=\"waterPump\">OFF</span>\n"
        "                </div>\n"
        "                <div class=\"actuator-item\">\n"
        "                    <span class=\"actuator-name\">Lighting</span>\n"
        "                    <span class=\"actuator-value\" id=\"lighting\">0%</span>\n"
        "                </div>\n"
        "                <div class=\"actuator-item\">\n"
        "                    <span class=\"actuator-name\">Ventilation</span>\n"
        "                    <span class=\"actuator-value\" id=\"ventilation\">0%</span>\n"
        "                </div>\n"
        "                <div class=\"actuator-item\">\n"
        "                    <span class=\"actuator-name\">Alarm</span>\n"
        "                    <span class=\"actuator-value\" id=\"alarm\">OFF</span>\n"
        "                </div>\n"
        "            </div>\n"
        "            <div style=\"margin-top: 20px;\">\n"
        "                <button class=\"btn btn-danger\" onclick=\"emergencyStop()\">🚨 Emergency Stop</button>\n"
        "            </div>\n"
        "        </div>\n"
        "        \n"
        "        <div class=\"refresh-info\">\n"
        "            Auto-refresh every 5 seconds | Last update: <span id=\"lastUpdate\">--</span>\n"
        "        </div>\n"
        "    </div>\n"
        "    \n"
        "    <script>\n"
        "        function updateData() {\n"
        "            fetch('/api/sensors')\n"
        "                .then(r => r.json())\n"
        "                .then(data => {\n"
        "                    document.getElementById('temperature').textContent = data.temperature.toFixed(1);\n"
        "                    document.getElementById('humidity').textContent = data.humidity.toFixed(1);\n"
        "                    document.getElementById('ammonia').textContent = data.ammonia_ppm.toFixed(1);\n"
        "                    document.getElementById('co2').textContent = data.co2_ppm.toFixed(0);\n"
        "                    document.getElementById('light').textContent = data.light_lux.toFixed(0);\n"
        "                    document.getElementById('waterLevel').textContent = data.water_level_percent.toFixed(1);\n"
        "                    document.getElementById('feedLevel').textContent = data.feed_level_percent.toFixed(1);\n"
        "                    document.getElementById('gas').textContent = data.gas_ppm.toFixed(1);\n"
        "                    document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();\n"
        "                });\n"
        "            \n"
        "            fetch('/api/actuators')\n"
        "                .then(r => r.json())\n"
        "                .then(data => {\n"
        "                    document.getElementById('exhaustFan').textContent = data.exhaust_fan_speed + '%';\n"
        "                    document.getElementById('inletFan').textContent = data.inlet_fan_speed + '%';\n"
        "                    document.getElementById('heater').textContent = data.heater_power + '%';\n"
        "                    document.getElementById('feeder').textContent = data.feeder_speed + '%';\n"
        "                    document.getElementById('waterPump').textContent = data.water_pump_state ? 'ON' : 'OFF';\n"
        "                    document.getElementById('lighting').textContent = data.lighting_intensity + '%';\n"
        "                    document.getElementById('ventilation').textContent = data.ventilation_position + '%';\n"
        "                    document.getElementById('alarm').textContent = data.alarm_state ? 'ON' : 'OFF';\n"
        "                });\n"
        "            \n"
        "            fetch('/api/status')\n"
        "                .then(r => r.json())\n"
        "                .then(data => {\n"
        "                    let hours = Math.floor(data.uptime_seconds / 3600);\n"
        "                    document.getElementById('uptime').textContent = hours + 'h';\n"
        "                    document.getElementById('freeHeap').textContent = Math.round(data.free_heap_size / 1024) + ' KB';\n"
        "                    \n"
        "                    let statusIndicator = document.getElementById('statusIndicator');\n"
        "                    let statusText = document.getElementById('statusText');\n"
        "                    statusIndicator.className = 'status-indicator status-normal';\n"
        "                    statusText.textContent = 'Normal';\n"
        "                    \n"
        "                    if (data.state === 1) { statusIndicator.className = 'status-indicator status-warning'; statusText.textContent = 'Warning'; }\n"
        "                    if (data.state === 2) { statusIndicator.className = 'status-indicator status-critical'; statusText.textContent = 'Critical'; }\n"
        "                    \n"
        "                    let alertBanner = document.getElementById('alertBanner');\n"
        "                    let alertMessage = document.getElementById('alertMessage');\n"
        "                    if (data.active_alert !== 0) {\n"
        "                        alertBanner.classList.add('active');\n"
        "                        alertMessage.textContent = 'Alert Type: ' + data.active_alert;\n"
        "                    } else {\n"
        "                        alertBanner.classList.remove('active');\n"
        "                    }\n"
        "                });\n"
        "        }\n"
        "        \n"
        "        function emergencyStop() {\n"
        "            if (confirm('Are you sure you want to emergency stop all actuators?')) {\n"
        "                fetch('/api/emergency_stop', { method: 'POST' })\n"
        "                    .then(r => r.json())\n"
        "                    .then(data => alert('Emergency stop executed'));\n"
        "            }\n"
        "        }\n"
        "        \n"
        "        updateData();\n"
        "        setInterval(updateData, 5000);\n"
        "    </script>\n"
        "</body>\n"
        "</html>\n";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

static esp_err_t sensors_get_handler(httpd_req_t *req) {
    update_cached_data();
    
    char response[512];
    int len = snprintf(response, sizeof(response),
        "{\"temperature\":%.2f,\"humidity\":%.2f,\"ammonia_ppm\":%.2f,\"co2_ppm\":%.2f,"
        "\"light_lux\":%.2f,\"water_level_percent\":%.2f,\"feed_level_percent\":%.2f,"
        "\"gas_ppm\":%.2f,\"gas_alarm\":%s,\"timestamp\":%lu,\"valid\":%s}",
        cached_sensor_data.temperature,
        cached_sensor_data.humidity,
        cached_sensor_data.ammonia_ppm,
        cached_sensor_data.co2_ppm,
        cached_sensor_data.light_lux,
        cached_sensor_data.water_level_percent,
        cached_sensor_data.feed_level_percent,
        cached_sensor_data.gas_ppm,
        cached_sensor_data.gas_alarm ? "true" : "false",
        (unsigned long)cached_sensor_data.timestamp,
        cached_sensor_data.valid ? "true" : "false"
    );
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, len);
    return ESP_OK;
}

static esp_err_t actuators_get_handler(httpd_req_t *req) {
    update_cached_data();
    
    char response[512];
    int len = snprintf(response, sizeof(response),
        "{\"exhaust_fan_speed\":%d,\"inlet_fan_speed\":%d,\"heater_power\":%d,"
        "\"feeder_speed\":%d,\"water_pump_state\":%d,\"lighting_intensity\":%d,"
        "\"ventilation_position\":%d,\"curtain_position\":%d,\"alarm_state\":%s}",
        cached_actuator_states.exhaust_fan_speed,
        cached_actuator_states.inlet_fan_speed,
        cached_actuator_states.heater_power,
        cached_actuator_states.feeder_speed,
        cached_actuator_states.water_pump_state,
        cached_actuator_states.lighting_intensity,
        cached_actuator_states.ventilation_position,
        cached_actuator_states.curtain_position,
        cached_actuator_states.alarm_state ? "true" : "false"
    );
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, len);
    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *req) {
    update_cached_data();
    
    char response[256];
    int len = snprintf(response, sizeof(response),
        "{\"state\":%d,\"active_alert\":%d,\"uptime_seconds\":%lu,\"free_heap_size\":%lu}",
        cached_system_status.state,
        cached_active_alert,
        (unsigned long)cached_system_status.uptime_seconds,
        (unsigned long)cached_system_status.free_heap_size
    );
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, len);
    return ESP_OK;
}

static esp_err_t alarm_toggle_handler(httpd_req_t *req) {
    extern esp_err_t actuator_manager_set_alarm(bool state);
    static bool alarm_state = false;
    alarm_state = !alarm_state;
    actuator_manager_set_alarm(alarm_state);
    
    char response[64];
    int len = snprintf(response, sizeof(response), "{\"alarm_state\":%s}", alarm_state ? "true" : "false");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, len);
    return ESP_OK;
}

static esp_err_t emergency_stop_handler(httpd_req_t *req) {
    extern esp_err_t actuator_manager_emergency_stop(void);
    actuator_manager_emergency_stop();
    
    const char *response = "{\"status\":\"ok\",\"message\":\"Emergency stop executed\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

static esp_err_t not_found_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "Not Found", 9);
    return ESP_FAIL;
}

esp_err_t web_server_init(const web_server_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&server_config, config, sizeof(web_server_config_t));
    
    data_mutex = xSemaphoreCreateMutex();
    if (data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create data mutex");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Web server initialized on port %d", config->port);
    return ESP_OK;
}

esp_err_t web_server_start(void) {
    if (server_running) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = server_config.port;
    config.max_open_sockets = server_config.max_connections;
    config.stack_size = server_config.stack_size;
    config.task_priority = 5;
    
    httpd_uri_t uri_handlers[] = {
        { .uri = "/",          .method = HTTP_GET,  .handler = root_get_handler },
        { .uri = "/api/sensors", .method = HTTP_GET,  .handler = sensors_get_handler },
        { .uri = "/api/actuators", .method = HTTP_GET, .handler = actuators_get_handler },
        { .uri = "/api/status", .method = HTTP_GET,  .handler = status_get_handler },
        { .uri = "/api/alarm/toggle", .method = HTTP_POST, .handler = alarm_toggle_handler },
        { .uri = "/api/emergency_stop", .method = HTTP_POST, .handler = emergency_stop_handler },
        { .uri = NULL }
    };
    
    esp_err_t ret = httpd_start(&server_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %d", ret);
        return ret;
    }
    
    for (int i = 0; uri_handlers[i].uri != NULL; i++) {
        if (httpd_register_uri_handler(server_handle, &uri_handlers[i]) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to register URI handler: %s", uri_handlers[i].uri);
        }
    }
    
    server_running = true;
    ESP_LOGI(TAG, "Web server started on port %d", server_config.port);
    
    return ESP_OK;
}

esp_err_t web_server_stop(void) {
    if (!server_running || server_handle == NULL) {
        return ESP_OK;
    }
    
    httpd_stop(server_handle);
    server_handle = NULL;
    server_running = false;
    
    ESP_LOGI(TAG, "Web server stopped");
    return ESP_OK;
}

bool web_server_is_running(void) {
    return server_running;
}

esp_err_t web_server_register_uri_handler(const httpd_uri_t *uri_handler) {
    if (server_handle == NULL || uri_handler == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return httpd_register_uri_handler(server_handle, uri_handler);
}

esp_err_t web_server_deinit(void) {
    web_server_stop();
    
    if (data_mutex != NULL) {
        vSemaphoreDelete(data_mutex);
        data_mutex = NULL;
    }
    
    ESP_LOGI(TAG, "Web server deinitialized");
    return ESP_OK;
}
