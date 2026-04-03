#ifndef FEEDING_SCHEDULE_H
#define FEEDING_SCHEDULE_H

#include "esp_err.h"
#include "../include/poultry_system_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Feeding schedule entry
 */
typedef struct {
    uint8_t hour;                   /*!< Hour of day (0-23) */
    uint8_t amount_grams;           /*!< Amount of feed in grams */
    bool enabled;                   /*!< Whether this entry is enabled */
} feeding_entry_t;

/**
 * @brief Initialize feeding schedule
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeding_schedule_init(void);

/**
 * @brief Set feeding schedule
 * 
 * @param schedule Array of 24 hourly feeding amounts
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeding_schedule_set(const uint8_t schedule[24]);

/**
 * @brief Get feeding schedule
 * 
 * @param schedule Array to store 24 hourly feeding amounts
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeding_schedule_get(uint8_t schedule[24]);

/**
 * @brief Set feeding amount for specific hour
 * 
 * @param hour Hour of day (0-23)
 * @param amount_grams Amount of feed in grams
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeding_schedule_set_hour(uint8_t hour, uint8_t amount_grams);

/**
 * @brief Get feeding amount for current hour
 * 
 * @param amount_grams Pointer to store feeding amount
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeding_schedule_get_current(uint8_t *amount_grams);

/**
 * @brief Check if feeding is due
 * 
 * @param current_hour Current hour of day
 * @param current_minute Current minute of hour
 * @param amount_grams Pointer to store feeding amount if due
 * @return true if feeding is due
 */
bool feeding_schedule_is_due(uint8_t current_hour, uint8_t current_minute, 
                            uint8_t *amount_grams);

/**
 * @brief Execute feeding
 * 
 * @param amount_grams Amount of feed in grams
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeding_schedule_execute(uint8_t amount_grams);

/**
 * @brief Enable/disable feeding schedule
 * 
 * @param enabled true to enable, false to disable
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeding_schedule_enable(bool enabled);

/**
 * @brief Check if feeding schedule is enabled
 * 
 * @return true if enabled
 */
bool feeding_schedule_is_enabled(void);

/**
 * @brief Deinitialize feeding schedule
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t feeding_schedule_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // FEEDING_SCHEDULE_H
