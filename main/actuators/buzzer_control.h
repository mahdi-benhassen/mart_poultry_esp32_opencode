#ifndef BUZZER_CONTROL_H
#define BUZZER_CONTROL_H

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buzzer alarm patterns
 */
typedef enum {
    BUZZER_PATTERN_CONTINUOUS = 0,  /*!< Continuous alarm */
    BUZZER_PATTERN_PULSE,           /*!< Pulsing alarm (on/off) */
    BUZZER_PATTERN_SIREN,           /*!< Siren pattern (rising/falling) */
    BUZZER_PATTERN_INTERMITTENT,    /*!< Intermittent beeps */
    BUZZER_PATTERN_EMERGENCY        /*!< Emergency rapid beeps */
} buzzer_pattern_t;

/**
 * @brief Buzzer configuration
 */
typedef struct {
    gpio_num_t gpio_num;            /*!< GPIO pin for buzzer */
    bool active_high;               /*!< Active high or low */
    uint16_t frequency_hz;          /*!< Buzzer frequency in Hz */
    uint8_t duty_cycle;             /*!< Duty cycle (0-100%) */
} buzzer_config_t;

/**
 * @brief Buzzer alarm state
 */
typedef struct {
    bool is_active;                 /*!< Alarm is active */
    buzzer_pattern_t pattern;       /*!< Current pattern */
    uint32_t start_time;            /*!< Alarm start time */
    uint32_t duration_ms;           /*!< Alarm duration (0 = infinite) */
    uint8_t intensity;              /*!< Alarm intensity (0-100%) */
} buzzer_state_t;

/**
 * @brief Initialize buzzer control
 * 
 * @param config Buzzer configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t buzzer_control_init(const buzzer_config_t *config);

/**
 * @brief Start buzzer alarm
 * 
 * @param pattern Alarm pattern
 * @param duration_ms Duration in milliseconds (0 = infinite)
 * @param intensity Alarm intensity (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t buzzer_alarm_start(buzzer_pattern_t pattern, uint32_t duration_ms, uint8_t intensity);

/**
 * @brief Stop buzzer alarm
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t buzzer_alarm_stop(void);

/**
 * @brief Check if alarm is active
 * 
 * @return true if alarm is active
 */
bool buzzer_alarm_is_active(void);

/**
 * @brief Get current alarm state
 * 
 * @param state Pointer to store alarm state
 * @return esp_err_t ESP_OK on success
 */
esp_err_t buzzer_alarm_get_state(buzzer_state_t *state);

/**
 * @brief Set alarm pattern
 * 
 * @param pattern Alarm pattern
 * @return esp_err_t ESP_OK on success
 */
esp_err_t buzzer_alarm_set_pattern(buzzer_pattern_t pattern);

/**
 * @brief Set alarm intensity
 * 
 * @param intensity Alarm intensity (0-100%)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t buzzer_alarm_set_intensity(uint8_t intensity);

/**
 * @brief Play test beep
 * 
 * @param duration_ms Beep duration in milliseconds
 * @return esp_err_t ESP_OK on success
 */
esp_err_t buzzer_test_beep(uint32_t duration_ms);

/**
 * @brief Deinitialize buzzer control
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t buzzer_control_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // BUZZER_CONTROL_H
