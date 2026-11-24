#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Default Firmware Information
#define FIRMWARE_CURRENT_VERSION    "1.0.0"
#define FIRMWARE_BUILD_HASH         "init-c3-v100"
#define DEFAULT_DEVICE_NAME         "MasterHelper"
#define DEFAULT_AP_SSID             "MasterHelper-AP"
#define DEFAULT_AP_PASS             "12345678"
#define DEFAULT_GITHUB_UPDATE_URL   "https://raw.githubusercontent.com/user/repo/main/version.json"

// Storage lengths
#define MAX_SSID_LEN        32
#define MAX_PASS_LEN        64
#define MAX_URL_LEN         256
#define MAX_NAME_LEN        32
#define MAX_VER_LEN         32
#define MAX_HASH_LEN        64

typedef struct {
    char wifi_ssid[MAX_SSID_LEN + 1];
    char wifi_pass[MAX_PASS_LEN + 1];
    char github_url[MAX_URL_LEN + 1];
    char device_name[MAX_NAME_LEN + 1];
    char current_version[MAX_VER_LEN + 1];
    char current_hash[MAX_HASH_LEN + 1];
} app_settings_t;

#ifdef __cplusplus
}
#endif
