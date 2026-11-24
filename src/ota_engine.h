#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_CHECKING,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_FLASHING,
    OTA_STATE_SUCCESS,
    OTA_STATE_FAILED
} ota_state_t;

typedef struct {
    char remote_version[32];
    char remote_hash[65];
    char bin_url[256];
    char notes[256];
    bool update_available;
    char message[128];
} ota_manifest_info_t;

typedef struct {
    ota_state_t state;
    int progress_percent;
    int total_bytes;
    int written_bytes;
    char message[128];
    char error_msg[128];
} ota_progress_t;

/**
 * @brief Initialize OTA engine mutexes and state.
 */
void ota_engine_init(void);

/**
 * @brief Query GitHub manifest JSON and check version/hash against running firmware.
 *
 * Checks if the remote version differs from current version, and if the hash differs.
 * Only if version changes (and hash != current) is update_available set to true.
 */
esp_err_t ota_engine_check(const char *manifest_url, ota_manifest_info_t *out_info);

/**
 * @brief Start OTA update in background task downloading binary from URL.
 */
esp_err_t ota_engine_start_update(const char *bin_url, const char *new_version, const char *new_hash);

/**
 * @brief Write chunk directly from Web UI manual binary upload.
 */
esp_err_t ota_engine_manual_begin(size_t total_size);
esp_err_t ota_engine_manual_write(const uint8_t *data, size_t length);
esp_err_t ota_engine_manual_end(void);

/**
 * @brief Get current OTA progress and state for Web UI polling.
 */
void ota_engine_get_progress(ota_progress_t *out_progress);

/**
 * @brief Get the last checked manifest info.
 */
void ota_engine_get_last_manifest(ota_manifest_info_t *out_info);

#ifdef __cplusplus
}
#endif
