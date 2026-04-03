#ifndef LIGHTING_SCHEDULE_H
#define LIGHTING_SCHEDULE_H

#include "esp_err.h"
#include "../include/poultry_system_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lighting schedule entry
 */
typedef struct {
    uint8_t hour;                   /*!< Hour of day (0-23) */
    uint8_t intensity;              /*!< Lighting intensity (0-100%) */
    bool enabled;                   /*!< Whether this entry is enabled */
} lighting_entry_t;

/**
 * @brief Initialize lighting schedule
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_schedule_init(void);

/**
 * @brief Set lighting schedule
 * 
 * @param schedule Array of 24 hourly lighting intensities
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_schedule_set(const uint8_t schedule[24]);

/**
 * @brief Get lighting schedule
 * 
 * @param schedule Array to store 24 hourly lighting intensities
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_schedule_get(uint8_t schedule[24]);

/**
 * @brief Set lighting intensity for specific hour
 * 
 * @param hour Hour of day (0-23)
 * @param intensity Lighting intensity (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_schedule_set_hour(uint8_t hour, uint8_t intensity);

/**
 * @brief Get lighting intensity for current hour
 * 
 * @param intensity Pointer to store lighting intensity
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_schedule_get_current(uint8_t *intensity);

/**
 * @brief Check if lighting change is due
 * 
 * @param current_hour Current hour of day
 * @param current_minute Current minute of hour
 * @param intensity Pointer to store lighting intensity if due
 * @return true if lighting change is due
 */
bool lighting_schedule_is_due(uint8_t current_hour, uint8_t current_minute, 
                             uint8_t *intensity);

/**
 * @brief Execute lighting change
 * 
 * @param intensity Lighting intensity (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_schedule_execute(uint8_t intensity);

/**
 * @brief Enable/disable lighting schedule
 * 
 * @param enabled true to enable, false to disable
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_schedule_enable(bool enabled);

/**
 * @brief Check if lighting schedule is enabled
 * 
 * @return true if enabled
 */
bool lighting_schedule_is_enabled(void);

/**
 * @brief Set day/night cycle
 * 
 * @param day_start_hour Hour when day starts (0-23)
 * @param day_end_hour Hour when day ends (0-23)
 * @param day_intensity Lighting intensity during day (0-100%)
 * @param night_intensity Lighting intensity during night (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_schedule_set_daynight(uint8_t day_start_hour, uint8_t day_end_hour,
                                         uint8_t day_intensity, uint8_t night_intensity);

/**
 * @brief Deinitialize lighting schedule
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lighting_schedule_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // LIGHTING_SCHEDULE_H
