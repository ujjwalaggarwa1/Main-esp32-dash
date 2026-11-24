#pragma once

#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_AP_MODE,
    WIFI_STATE_AP_STA_MODE
} wifi_state_t;

/**
 * @brief Initialize WiFi driver, event handlers, and default netifs.
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Start WiFi according to saved settings (attempts STA, fallback to SoftAP if needed).
 */
esp_err_t wifi_manager_start(void);

/**
 * @brief Check if Station is connected to an AP and has acquired an IP.
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Check if SoftAP mode is active.
 */
bool wifi_manager_is_ap_active(void);

/**
 * @brief Get the current IP address string (either STA IP or SoftAP IP).
 */
void wifi_manager_get_ip_str(char *buf, size_t max_len);

/**
 * @brief Get the current RSSI in dBm (-100 to 0).
 */
int8_t wifi_manager_get_rssi(void);

/**
 * @brief Get current WiFi state enum.
 */
wifi_state_t wifi_manager_get_state(void);

/**
 * @brief Reconnect WiFi with newly updated credentials.
 */
esp_err_t wifi_manager_reconnect(const char *ssid, const char *pass);

#ifdef __cplusplus
}
#endif
