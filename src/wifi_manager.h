#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_AP_MODE,
    WIFI_STATE_AP_STA_MODE
} wifi_state_t;

esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_start(void);
bool wifi_manager_is_connected(void);
bool wifi_manager_is_ap_active(void);
void wifi_manager_get_ip_str(char *buf, size_t max_len);
int8_t wifi_manager_get_rssi(void);
wifi_state_t wifi_manager_get_state(void);
esp_err_t wifi_manager_reconnect(const char *ssid, const char *pass);

#ifdef __cplusplus
}
#endif
