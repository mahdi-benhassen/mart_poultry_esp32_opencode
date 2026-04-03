#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi manager configuration
 */
typedef struct {
    char ssid[WIFI_SSID_MAX_LEN];       /*!< WiFi SSID */
    char password[WIFI_PASSWORD_MAX_LEN]; /*!< WiFi password */
    uint8_t max_retry;                  /*!< Maximum connection retries */
    uint32_t timeout_ms;                /*!< Connection timeout in milliseconds */
} poultry_wifi_config_t;

/**
 * @brief WiFi connection status
 */
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,       /*!< Disconnected */
    WIFI_STATUS_CONNECTING,             /*!< Connecting */
    WIFI_STATUS_CONNECTED,              /*!< Connected */
    WIFI_STATUS_GOT_IP,                 /*!< Got IP address */
    WIFI_STATUS_ERROR                   /*!< Error */
} wifi_status_t;

/**
 * @brief Initialize WiFi manager
 * 
 * @param config WiFi configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_init(const wifi_config_t *config);

/**
 * @brief Connect to WiFi
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Disconnect from WiFi
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get WiFi status
 * 
 * @return wifi_status_t Current WiFi status
 */
wifi_status_t wifi_manager_get_status(void);

/**
 * @brief Check if connected to WiFi
 * 
 * @return true if connected
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Get IP address
 * 
 * @param ip_str Buffer to store IP address string
 * @param max_len Maximum length of buffer
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_get_ip(char *ip_str, size_t max_len);

/**
 * @brief Get RSSI (signal strength)
 * 
 * @param rssi Pointer to store RSSI value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_get_rssi(int8_t *rssi);

/**
 * @brief Scan for available networks
 * 
 * @param ap_records Array to store AP records
 * @param max_ap Maximum number of APs to scan
 * @param ap_count Pointer to store actual number of APs found
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_scan(wifi_ap_record_t *ap_records, uint16_t max_ap, 
                            uint16_t *ap_count);

/**
 * @brief Set WiFi configuration
 * 
 * @param config WiFi configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_set_config(const wifi_config_t *config);

/**
 * @brief Get WiFi configuration
 * 
 * @param config Pointer to store WiFi configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_get_config(wifi_config_t *config);

/**
 * @brief Start WiFi in AP mode
 * 
 * @param ssid AP SSID
 * @param password AP password
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_start_ap(const char *ssid, const char *password);

/**
 * @brief Stop WiFi AP mode
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_stop_ap(void);

/**
 * @brief Deinitialize WiFi manager
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
