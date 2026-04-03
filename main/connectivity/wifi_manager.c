#include "wifi_manager.h"
#include "../include/poultry_system_config.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "WIFI_MANAGER";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t wifi_event_group;
static poultry_wifi_config_t wifi_config = {0};
static wifi_status_t wifi_status = WIFI_STATUS_DISCONNECTED;
static int retry_count = 0;
static bool initialized = false;
static bool ap_mode = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        wifi_status = WIFI_STATUS_CONNECTING;
        ESP_LOGI(TAG, "WiFi station started, connecting...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < wifi_config.max_retry) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "Retrying to connect to WiFi (%d/%d)", 
                     retry_count, wifi_config.max_retry);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            wifi_status = WIFI_STATUS_ERROR;
            ESP_LOGE(TAG, "Failed to connect to WiFi");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_count = 0;
        wifi_status = WIFI_STATUS_GOT_IP;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station connected to AP, AID=%d", event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station disconnected from AP, AID=%d", event->aid);
    }
}

esp_err_t wifi_manager_init(const poultry_wifi_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&wifi_config, config, sizeof(poultry_wifi_config_t));
    
    // Set defaults
    if (wifi_config.max_retry == 0) {
        wifi_config.max_retry = 5;
    }
    if (wifi_config.timeout_ms == 0) {
        wifi_config.timeout_ms = 10000;
    }
    
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Create WiFi event group
    wifi_event_group = xEventGroupCreate();
    
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                               &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                               &wifi_event_handler, NULL));
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Set WiFi mode to station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // Set WiFi configuration
    wifi_config_t esp_wifi_cfg = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)esp_wifi_cfg.sta.ssid, wifi_config.ssid, sizeof(esp_wifi_cfg.sta.ssid) - 1);
    strncpy((char *)esp_wifi_cfg.sta.password, wifi_config.password, sizeof(esp_wifi_cfg.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &esp_wifi_cfg));
    
    initialized = true;
    ESP_LOGI(TAG, "WiFi manager initialized");
    
    return ESP_OK;
}

esp_err_t wifi_manager_connect(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (ap_mode) {
        ESP_LOGE(TAG, "Cannot connect in AP mode");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi: %s", wifi_config.ssid);
    wifi_status = WIFI_STATUS_CONNECTING;
    
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(wifi_config.timeout_ms));
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to WiFi: %s", wifi_config.ssid);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to WiFi: %s", wifi_config.ssid);
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "WiFi connection timeout");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t wifi_manager_disconnect(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    wifi_status = WIFI_STATUS_DISCONNECTED;
    ESP_LOGI(TAG, "Disconnected from WiFi");
    
    return ESP_OK;
}

wifi_status_t wifi_manager_get_status(void) {
    return wifi_status;
}

bool wifi_manager_is_connected(void) {
    return (wifi_status == WIFI_STATUS_GOT_IP);
}

esp_err_t wifi_manager_get_ip(char *ip_str, size_t max_len) {
    if (!initialized || ip_str == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (wifi_status != WIFI_STATUS_GOT_IP) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    
    esp_netif_ip_info_t ip_info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip_info));
    
    snprintf(ip_str, max_len, IPSTR, IP2STR(&ip_info.ip));
    
    return ESP_OK;
}

esp_err_t wifi_manager_get_rssi(int8_t *rssi) {
    if (!initialized || rssi == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (wifi_status != WIFI_STATUS_GOT_IP) {
        return ESP_ERR_INVALID_STATE;
    }
    
    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    
    *rssi = ap_info.rssi;
    
    return ESP_OK;
}

esp_err_t wifi_manager_scan(wifi_ap_record_t *ap_records, uint16_t max_ap, 
                            uint16_t *ap_count) {
    if (!initialized || ap_records == NULL || ap_count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Starting WiFi scan...");
    
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };
    
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&max_ap, ap_records));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(ap_count));
    
    ESP_LOGI(TAG, "WiFi scan complete, found %d networks", *ap_count);
    
    return ESP_OK;
}

esp_err_t wifi_manager_set_config(const poultry_wifi_config_t *config) {
    if (!initialized || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&wifi_config, config, sizeof(poultry_wifi_config_t));
    
    // Update WiFi configuration
    wifi_config_t esp_wifi_cfg = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)esp_wifi_cfg.sta.ssid, wifi_config.ssid, sizeof(esp_wifi_cfg.sta.ssid) - 1);
    strncpy((char *)esp_wifi_cfg.sta.password, wifi_config.password, sizeof(esp_wifi_cfg.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &esp_wifi_cfg));
    
    ESP_LOGI(TAG, "WiFi configuration updated");
    return ESP_OK;
}

esp_err_t wifi_manager_get_config(poultry_wifi_config_t *config) {
    if (!initialized || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &wifi_config, sizeof(poultry_wifi_config_t));
    return ESP_OK;
}

esp_err_t wifi_manager_start_ap(const char *ssid, const char *password) {
    if (!initialized || ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Starting WiFi AP: %s", ssid);
    
    // Stop station mode if running
    if (wifi_status != WIFI_STATUS_DISCONNECTED) {
        esp_wifi_stop();
    }
    
    // Set AP mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    
    // Configure AP
    wifi_config_t esp_ap_cfg = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    strncpy((char *)esp_ap_cfg.ap.ssid, ssid, sizeof(esp_ap_cfg.ap.ssid) - 1);
    esp_ap_cfg.ap.ssid_len = strlen(ssid);
    
    if (password != NULL && strlen(password) > 0) {
        strncpy((char *)esp_ap_cfg.ap.password, password, sizeof(esp_ap_cfg.ap.password) - 1);
    } else {
        esp_ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &esp_ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ap_mode = true;
    wifi_status = WIFI_STATUS_CONNECTED;
    
    ESP_LOGI(TAG, "WiFi AP started: %s", ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_stop_ap(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!ap_mode) {
        return ESP_OK;
    }
    
    ESP_ERROR_CHECK(esp_wifi_stop());
    ap_mode = false;
    wifi_status = WIFI_STATUS_DISCONNECTED;
    
    ESP_LOGI(TAG, "WiFi AP stopped");
    return ESP_OK;
}

esp_err_t wifi_manager_deinit(void) {
    if (initialized) {
        esp_wifi_stop();
        esp_wifi_deinit();
        vEventGroupDelete(wifi_event_group);
        initialized = false;
        ESP_LOGI(TAG, "WiFi manager deinitialized");
    }
    return ESP_OK;
}
