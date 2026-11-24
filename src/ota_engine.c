#include "ota_engine.h"
#include "config_manager.h"
#include <string.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <cJSON.h>

static const char *TAG = "OTA_ENGINE";

#define OTA_BUFF_SIZE 2048

static SemaphoreHandle_t s_ota_mutex = NULL;
static ota_progress_t s_ota_progress;
static ota_manifest_info_t s_last_manifest;

typedef struct {
    char bin_url[256];
    char new_version[32];
    char new_hash[65];
} ota_task_params_t;

static esp_ota_handle_t s_manual_ota_handle = 0;
static const esp_partition_t *s_manual_update_partition = NULL;
static bool s_manual_ota_active = false;

void ota_engine_init(void) {
    if (!s_ota_mutex) {
        s_ota_mutex = xSemaphoreCreateMutex();
    }
    memset(&s_ota_progress, 0, sizeof(s_ota_progress));
    s_ota_progress.state = OTA_STATE_IDLE;
    strncpy(s_ota_progress.message, "Ready", sizeof(s_ota_progress.message));

    memset(&s_last_manifest, 0, sizeof(s_last_manifest));
}

esp_err_t ota_engine_check(const char *manifest_url, ota_manifest_info_t *out_info) {
    if (!manifest_url || strlen(manifest_url) == 0) {
        if (out_info) {
            strncpy(out_info->message, "GitHub update URL is empty", sizeof(out_info->message));
        }
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Fetching OTA manifest from: %s", manifest_url);

    esp_http_client_config_t config = {
        .url = manifest_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
        .keep_alive_enable = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client for manifest check");
        return ESP_FAIL;
    }

    // Support redirects (GitHub raw redirects to AWS S3)
    esp_http_client_set_header(client, "User-Agent", "ESP32-C3-MasterHelper");
    esp_http_client_set_header(client, "Accept", "application/json");

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        if (out_info) {
            snprintf(out_info->message, sizeof(out_info->message), "Connection failed: %s", esp_err_to_name(err));
        }
        return err;
    }

    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status: %d, Content-Length: %d", status_code, content_length);

    if (status_code < 200 || status_code >= 300) {
        esp_http_client_cleanup(client);
        if (out_info) {
            snprintf(out_info->message, sizeof(out_info->message), "HTTP error code: %d", status_code);
        }
        return ESP_FAIL;
    }

    // Read response buffer (max 4KB for manifest JSON)
    char response_buffer[4096] = {0};
    int total_read = 0;
    while (total_read < sizeof(response_buffer) - 1) {
        int read_len = esp_http_client_read(client, response_buffer + total_read, sizeof(response_buffer) - 1 - total_read);
        if (read_len <= 0) break;
        total_read += read_len;
    }
    response_buffer[total_read] = '\0';
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "Manifest content: %s", response_buffer);

    // Parse JSON
    cJSON *root = cJSON_Parse(response_buffer);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse error");
        if (out_info) {
            strncpy(out_info->message, "Invalid JSON format in manifest", sizeof(out_info->message));
        }
        return ESP_ERR_INVALID_RESPONSE;
    }

    ota_manifest_info_t info = {0};

    cJSON *j_ver = cJSON_GetObjectItem(root, "version");
    if (j_ver && cJSON_IsString(j_ver)) {
        strncpy(info.remote_version, j_ver->valuestring, sizeof(info.remote_version) - 1);
    }

    cJSON *j_hash = cJSON_GetObjectItem(root, "hash");
    if (j_hash && cJSON_IsString(j_hash)) {
        strncpy(info.remote_hash, j_hash->valuestring, sizeof(info.remote_hash) - 1);
    }

    cJSON *j_url = cJSON_GetObjectItem(root, "url");
    if (!j_url) j_url = cJSON_GetObjectItem(root, "bin_url");
    if (j_url && cJSON_IsString(j_url)) {
        strncpy(info.bin_url, j_url->valuestring, sizeof(info.bin_url) - 1);
    }

    cJSON *j_notes = cJSON_GetObjectItem(root, "notes");
    if (j_notes && cJSON_IsString(j_notes)) {
        strncpy(info.notes, j_notes->valuestring, sizeof(info.notes) - 1);
    }

    cJSON_Delete(root);

    // Get current device settings
    app_settings_t current;
    config_manager_get(&current);

    bool version_changed = (strcmp(info.remote_version, current.current_version) != 0);
    bool hash_mismatch   = (strcmp(info.remote_hash, current.current_hash) != 0);

    // Rule: Update only if version number changed AND hash is different from current!
    if (version_changed && hash_mismatch) {
        info.update_available = true;
        snprintf(info.message, sizeof(info.message),
                 "Update available: v%s (Current: v%s)", info.remote_version, current.current_version);
    } else if (!version_changed) {
        info.update_available = false;
        snprintf(info.message, sizeof(info.message),
                 "Firmware is up to date (Version %s matches)", current.current_version);
    } else {
        info.update_available = false;
        snprintf(info.message, sizeof(info.message),
                 "Hash matches current build (%s). No update needed.", current.current_hash);
    }

    ESP_LOGI(TAG, "Manifest check complete: %s (Update: %s)",
             info.message, info.update_available ? "YES" : "NO");

    xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
    memcpy(&s_last_manifest, &info, sizeof(ota_manifest_info_t));
    xSemaphoreGive(s_ota_mutex);

    if (out_info) {
        memcpy(out_info, &info, sizeof(ota_manifest_info_t));
    }

    return ESP_OK;
}

static void ota_download_task(void *pvParameters) {
    ota_task_params_t *params = (ota_task_params_t *)pvParameters;
    char bin_url[256];
    char new_ver[32];
    char new_hash[65];
    strncpy(bin_url, params->bin_url, sizeof(bin_url));
    strncpy(new_ver, params->new_version, sizeof(new_ver));
    strncpy(new_hash, params->new_hash, sizeof(new_hash));
    free(params);

    ESP_LOGI(TAG, "Starting OTA Download from %s", bin_url);

    xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
    s_ota_progress.state = OTA_STATE_DOWNLOADING;
    s_ota_progress.progress_percent = 0;
    s_ota_progress.written_bytes = 0;
    strncpy(s_ota_progress.message, "Connecting to server...", sizeof(s_ota_progress.message));
    s_ota_progress.error_msg[0] = '\0';
    xSemaphoreGive(s_ota_mutex);

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(TAG, "No OTA partition found");
        xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
        s_ota_progress.state = OTA_STATE_FAILED;
        strncpy(s_ota_progress.error_msg, "No valid OTA partition found", sizeof(s_ota_progress.error_msg));
        xSemaphoreGive(s_ota_mutex);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx",
             update_partition->subtype, update_partition->address);

    esp_http_client_config_t http_cfg = {
        .url = bin_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
        .keep_alive_enable = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
        s_ota_progress.state = OTA_STATE_FAILED;
        strncpy(s_ota_progress.error_msg, "Failed to init HTTP client", sizeof(s_ota_progress.error_msg));
        xSemaphoreGive(s_ota_mutex);
        vTaskDelete(NULL);
        return;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
        s_ota_progress.state = OTA_STATE_FAILED;
        snprintf(s_ota_progress.error_msg, sizeof(s_ota_progress.error_msg), "Connection error: %s", esp_err_to_name(err));
        xSemaphoreGive(s_ota_mutex);
        vTaskDelete(NULL);
        return;
    }

    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    if (status_code < 200 || status_code >= 300) {
        ESP_LOGE(TAG, "Server returned HTTP %d", status_code);
        esp_http_client_cleanup(client);
        xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
        s_ota_progress.state = OTA_STATE_FAILED;
        snprintf(s_ota_progress.error_msg, sizeof(s_ota_progress.error_msg), "HTTP %d error", status_code);
        xSemaphoreGive(s_ota_mutex);
        vTaskDelete(NULL);
        return;
    }

    xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
    s_ota_progress.total_bytes = content_length;
    s_ota_progress.state = OTA_STATE_DOWNLOADING;
    strncpy(s_ota_progress.message, "Writing firmware to flash...", sizeof(s_ota_progress.message));
    xSemaphoreGive(s_ota_mutex);

    esp_ota_handle_t ota_handle = 0;
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
        s_ota_progress.state = OTA_STATE_FAILED;
        snprintf(s_ota_progress.error_msg, sizeof(s_ota_progress.error_msg), "OTA begin failed: %s", esp_err_to_name(err));
        xSemaphoreGive(s_ota_mutex);
        vTaskDelete(NULL);
        return;
    }

    char *upgrade_buf = (char *)malloc(OTA_BUFF_SIZE);
    if (!upgrade_buf) {
        esp_ota_end(ota_handle);
        esp_http_client_cleanup(client);
        xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
        s_ota_progress.state = OTA_STATE_FAILED;
        strncpy(s_ota_progress.error_msg, "Out of memory", sizeof(s_ota_progress.error_msg));
        xSemaphoreGive(s_ota_mutex);
        vTaskDelete(NULL);
        return;
    }

    int binary_file_len = 0;
    while (1) {
        int data_read = esp_http_client_read(client, upgrade_buf, OTA_BUFF_SIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "SSL/HTTP data read error");
            break;
        } else if (data_read > 0) {
            err = esp_ota_write(ota_handle, (const void *)upgrade_buf, data_read);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write error: %s", esp_err_to_name(err));
                break;
            }
            binary_file_len += data_read;

            xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
            s_ota_progress.written_bytes = binary_file_len;
            if (content_length > 0) {
                s_ota_progress.progress_percent = (binary_file_len * 100) / content_length;
            }
            snprintf(s_ota_progress.message, sizeof(s_ota_progress.message),
                     "Downloading & Flashing: %d%%", s_ota_progress.progress_percent);
            xSemaphoreGive(s_ota_mutex);
        } else if (data_read == 0) {
            // End of stream
            break;
        }
    }

    free(upgrade_buf);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || (content_length > 0 && binary_file_len != content_length)) {
        esp_ota_abort(ota_handle);
        ESP_LOGE(TAG, "OTA failed during write/download. Read: %d, Expected: %d", binary_file_len, content_length);
        xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
        s_ota_progress.state = OTA_STATE_FAILED;
        strncpy(s_ota_progress.error_msg, "Incomplete download or write error", sizeof(s_ota_progress.error_msg));
        xSemaphoreGive(s_ota_mutex);
        vTaskDelete(NULL);
        return;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
        xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
        s_ota_progress.state = OTA_STATE_FAILED;
        snprintf(s_ota_progress.error_msg, sizeof(s_ota_progress.error_msg), "Image validation failed: %s", esp_err_to_name(err));
        xSemaphoreGive(s_ota_mutex);
        vTaskDelete(NULL);
        return;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
        xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
        s_ota_progress.state = OTA_STATE_FAILED;
        snprintf(s_ota_progress.error_msg, sizeof(s_ota_progress.error_msg), "Set boot partition error: %s", esp_err_to_name(err));
        xSemaphoreGive(s_ota_mutex);
        vTaskDelete(NULL);
        return;
    }

    // Persist new version and hash to NVS before restart!
    config_manager_set_version_info(new_ver, new_hash);

    ESP_LOGI(TAG, "OTA Update Successful! Device will reboot in 2 seconds...");
    xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
    s_ota_progress.state = OTA_STATE_SUCCESS;
    s_ota_progress.progress_percent = 100;
    strncpy(s_ota_progress.message, "Update complete! Rebooting...", sizeof(s_ota_progress.message));
    xSemaphoreGive(s_ota_mutex);

    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}

esp_err_t ota_engine_start_update(const char *bin_url, const char *new_version, const char *new_hash) {
    if (!bin_url || strlen(bin_url) == 0) return ESP_ERR_INVALID_ARG;

    ota_task_params_t *params = (ota_task_params_t *)malloc(sizeof(ota_task_params_t));
    if (!params) return ESP_ERR_NO_MEM;

    strncpy(params->bin_url, bin_url, sizeof(params->bin_url) - 1);
    strncpy(params->new_version, new_version ? new_version : "updated", sizeof(params->new_version) - 1);
    strncpy(params->new_hash, new_hash ? new_hash : "new_hash", sizeof(params->new_hash) - 1);

    BaseType_t ret = xTaskCreate(ota_download_task, "ota_task", 8192, params, 5, NULL);
    if (ret != pdPASS) {
        free(params);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t ota_engine_manual_begin(size_t total_size) {
    s_manual_update_partition = esp_ota_get_next_update_partition(NULL);
    if (!s_manual_update_partition) {
        return ESP_ERR_NOT_FOUND;
    }
    esp_err_t err = esp_ota_begin(s_manual_update_partition, total_size, &s_manual_ota_handle);
    if (err == ESP_OK) {
        s_manual_ota_active = true;
    }
    return err;
}

esp_err_t ota_engine_manual_write(const uint8_t *data, size_t length) {
    if (!s_manual_ota_active) return ESP_ERR_INVALID_STATE;
    return esp_ota_write(s_manual_ota_handle, data, length);
}

esp_err_t ota_engine_manual_end(void) {
    if (!s_manual_ota_active) return ESP_ERR_INVALID_STATE;
    s_manual_ota_active = false;

    esp_err_t err = esp_ota_end(s_manual_ota_handle);
    if (err != ESP_OK) return err;

    err = esp_ota_set_boot_partition(s_manual_update_partition);
    if (err == ESP_OK) {
        config_manager_set_version_info("manual_web_upload", "manual_flash");
    }
    return err;
}

void ota_engine_get_progress(ota_progress_t *out_progress) {
    if (!out_progress) return;
    xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
    memcpy(out_progress, &s_ota_progress, sizeof(ota_progress_t));
    xSemaphoreGive(s_ota_mutex);
}

void ota_engine_get_last_manifest(ota_manifest_info_t *out_info) {
    if (!out_info) return;
    xSemaphoreTake(s_ota_mutex, portMAX_DELAY);
    memcpy(out_info, &s_last_manifest, sizeof(ota_manifest_info_t));
    xSemaphoreGive(s_ota_mutex);
}
