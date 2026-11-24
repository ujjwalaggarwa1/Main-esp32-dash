#pragma once

#include <esp_err.h>
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize NVS and load saved settings or defaults.
 */
esp_err_t config_manager_init(void);

/**
 * @brief Get a copy of the current in-memory settings.
 */
esp_err_t config_manager_get(app_settings_t *out_settings);

/**
 * @brief Save all settings into NVS.
 */
esp_err_t config_manager_save(const app_settings_t *settings);

/**
 * @brief Set and persist WiFi credentials.
 */
esp_err_t config_manager_set_wifi(const char *ssid, const char *pass);

/**
 * @brief Set and persist GitHub OTA manifest URL.
 */
esp_err_t config_manager_set_github_url(const char *url);

/**
 * @brief Set and persist Current Version and Hash (after successful OTA).
 */
esp_err_t config_manager_set_version_info(const char *version, const char *hash);

/**
 * @brief Reset settings to factory defaults.
 */
esp_err_t config_manager_reset_defaults(void);

#ifdef __cplusplus
}
#endif
