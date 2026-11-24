#pragma once

#include "app_config.h"
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t config_manager_init(void);
esp_err_t config_manager_get(app_settings_t *out_settings);
esp_err_t config_manager_save(const app_settings_t *settings);
esp_err_t config_manager_set_wifi(const char *ssid, const char *pass);
esp_err_t config_manager_set_github_url(const char *url);
esp_err_t config_manager_set_version_info(const char *version, const char *hash);
esp_err_t config_manager_reset_defaults(void);

#ifdef __cplusplus
}
#endif
