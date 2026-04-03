#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Web server configuration
 */
typedef struct {
    uint16_t port;                  /*!< Server port */
    uint8_t max_connections;        /*!< Maximum connections */
    size_t stack_size;              /*!< Task stack size */
} web_server_config_t;

/**
 * @brief Initialize web server
 * 
 * @param config Server configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t web_server_init(const web_server_config_t *config);

/**
 * @brief Start web server
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t web_server_start(void);

/**
 * @brief Stop web server
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t web_server_stop(void);

/**
 * @brief Check if web server is running
 * 
 * @return true if running
 */
bool web_server_is_running(void);

/**
 * @brief Register URI handler
 * 
 * @param uri_handler URI handler structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t web_server_register_uri_handler(const httpd_uri_t *uri_handler);

/**
 * @brief Deinitialize web server
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t web_server_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H
