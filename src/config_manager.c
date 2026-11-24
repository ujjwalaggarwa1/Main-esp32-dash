#include "config_manager.h"
#include <string.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

static const char *TAG = "CONFIG_MGR";
#define NVS_NAMESPACE "helper_cfg"

static app_settings_t s_settings;
static bool s_initialized = false;

esp_err_t config_manager_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing corrupted/old NVS partition...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
        return err;
    }

    nvs_handle_t handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }

    // Initialize with default values
    memset(&s_settings, 0, sizeof(s_settings));
    strncpy(s_settings.device_name, DEFAULT_DEVICE_NAME, MAX_NAME_LEN);
    strncpy(s_settings.github_url, DEFAULT_GITHUB_UPDATE_URL, MAX_URL_LEN);
    strncpy(s_settings.current_version, FIRMWARE_CURRENT_VERSION, MAX_VER_LEN);
    strncpy(s_settings.current_hash, FIRMWARE_BUILD_HASH, MAX_HASH_LEN);

    // Read stored values if present
    size_t len;
    len = sizeof(s_settings.wifi_ssid);
    nvs_get_str(handle, "wifi_ssid", s_settings.wifi_ssid, &len);

    len = sizeof(s_settings.wifi_pass);
    nvs_get_str(handle, "wifi_pass", s_settings.wifi_pass, &len);

    len = sizeof(s_settings.github_url);
    nvs_get_str(handle, "github_url", s_settings.github_url, &len);

    len = sizeof(s_settings.device_name);
    nvs_get_str(handle, "device_name", s_settings.device_name, &len);

    len = sizeof(s_settings.current_version);
    if (nvs_get_str(handle, "cur_ver", s_settings.current_version, &len) != ESP_OK) {
        // First boot or unwritten: write code-defined FIRMWARE_CURRENT_VERSION
        strncpy(s_settings.current_version, FIRMWARE_CURRENT_VERSION, MAX_VER_LEN);
        nvs_set_str(handle, "cur_ver", s_settings.current_version);
    }

    len = sizeof(s_settings.current_hash);
    if (nvs_get_str(handle, "cur_hash", s_settings.current_hash, &len) != ESP_OK) {
        strncpy(s_settings.current_hash, FIRMWARE_BUILD_HASH, MAX_HASH_LEN);
        nvs_set_str(handle, "cur_hash", s_settings.current_hash);
    }

    nvs_commit(handle);
    nvs_close(handle);

    s_initialized = true;
    ESP_LOGI(TAG, "Config initialized. Version: %s, Hash: %s, SSID: '%s', GitHub URL: %s",
             s_settings.current_version, s_settings.current_hash, s_settings.wifi_ssid, s_settings.github_url);
    return ESP_OK;
}

esp_err_t config_manager_get(app_settings_t *out_settings) {
    if (!out_settings) return ESP_ERR_INVALID_ARG;
    if (!s_initialized) {
        esp_err_t err = config_manager_init();
        if (err != ESP_OK) return err;
    }
    memcpy(out_settings, &s_settings, sizeof(app_settings_t));
    return ESP_OK;
}

esp_err_t config_manager_save(const app_settings_t *settings) {
    if (!settings) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    nvs_set_str(handle, "wifi_ssid", settings->wifi_ssid);
    nvs_set_str(handle, "wifi_pass", settings->wifi_pass);
    nvs_set_str(handle, "github_url", settings->github_url);
    nvs_set_str(handle, "device_name", settings->device_name);
    nvs_set_str(handle, "cur_ver", settings->current_version);
    nvs_set_str(handle, "cur_hash", settings->current_hash);

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err == ESP_OK) {
        memcpy(&s_settings, settings, sizeof(app_settings_t));
        ESP_LOGI(TAG, "Settings successfully saved to NVS.");
    }
    return err;
}

esp_err_t config_manager_set_wifi(const char *ssid, const char *pass) {
    if (!ssid) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    strncpy(s_settings.wifi_ssid, ssid, MAX_SSID_LEN);
    s_settings.wifi_ssid[MAX_SSID_LEN] = '\0';
    nvs_set_str(handle, "wifi_ssid", s_settings.wifi_ssid);

    if (pass) {
        strncpy(s_settings.wifi_pass, pass, MAX_PASS_LEN);
        s_settings.wifi_pass[MAX_PASS_LEN] = '\0';
        nvs_set_str(handle, "wifi_pass", s_settings.wifi_pass);
    }

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t config_manager_set_github_url(const char *url) {
    if (!url) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    strncpy(s_settings.github_url, url, MAX_URL_LEN);
    s_settings.github_url[MAX_URL_LEN] = '\0';
    nvs_set_str(handle, "github_url", s_settings.github_url);

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t config_manager_set_version_info(const char *version, const char *hash) {
    if (!version || !hash) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    strncpy(s_settings.current_version, version, MAX_VER_LEN);
    s_settings.current_version[MAX_VER_LEN] = '\0';
    nvs_set_str(handle, "cur_ver", s_settings.current_version);

    strncpy(s_settings.current_hash, hash, MAX_HASH_LEN);
    s_settings.current_hash[MAX_HASH_LEN] = '\0';
    nvs_set_str(handle, "cur_hash", s_settings.current_hash);

    err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t config_manager_reset_defaults(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    nvs_erase_all(handle);
    nvs_commit(handle);
    nvs_close(handle);

    return config_manager_init();
}
