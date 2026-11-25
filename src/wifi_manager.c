#include "wifi_manager.h"
#include "config_manager.h"
#include <string.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_mac.h>
#include <mdns.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

static const char *TAG = "WIFI_MGR";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAX_RETRY_COUNT    5

static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;

static int s_retry_count = 0;
static bool s_is_connected = false;
static bool s_ap_active = false;
static char s_ip_str[32] = "0.0.0.0";
static wifi_state_t s_wifi_state = WIFI_STATE_DISCONNECTED;

static void start_softap(void);

static void init_mdns(void) {
    static bool s_mdns_initialized = false;
    if (s_mdns_initialized) return;

    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mDNS init failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_ERROR_CHECK(mdns_hostname_set("master"));
    ESP_ERROR_CHECK(mdns_instance_name_set("Master Helper ESP32-C3"));
    mdns_service_add("MasterHelper-Web", "_http", "_tcp", 80, NULL, 0);
    s_mdns_initialized = true;
    ESP_LOGI(TAG, "mDNS responder running! Accessible at: http://master.local");
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi STA started, connecting...");
        s_wifi_state = WIFI_STATE_CONNECTING;
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_is_connected = false;
        if (s_retry_count < MAX_RETRY_COUNT) {
            s_retry_count++;
            ESP_LOGW(TAG, "WiFi disconnected, retry %d of %d...", s_retry_count, MAX_RETRY_COUNT);
            esp_wifi_connect();
        } else {
            ESP_LOGW(TAG, "WiFi connection failed after %d retries. Starting SoftAP fallback...", MAX_RETRY_COUNT);
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            start_softap();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        esp_ip4addr_ntoa(&event->ip_info.ip, s_ip_str, sizeof(s_ip_str));
        ESP_LOGI(TAG, "WiFi Connected! Acquired IP: %s", s_ip_str);
        s_retry_count = 0;
        s_is_connected = true;
        s_wifi_state = WIFI_STATE_AP_STA_MODE;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        init_mdns();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station %02x:%02x:%02x:%02x:%02x:%02x joined SoftAP, AID=%d",
                 event->mac[0], event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5], event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station %02x:%02x:%02x:%02x:%02x:%02x left SoftAP, AID=%d",
                 event->mac[0], event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5], event->aid);
    }
}

static void start_softap(void) {
    if (s_ap_active) return;

    ESP_LOGI(TAG, "Activating SoftAP configuration mode...");
    if (!s_ap_netif) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
    }
    esp_netif_set_hostname(s_ap_netif, "master");

    wifi_config_t ap_config = {
        .ap = {
            .ssid = DEFAULT_AP_SSID,
            .ssid_len = strlen(DEFAULT_AP_SSID),
            .channel = 1,
            .password = DEFAULT_AP_PASS,
            .max_connection = 4,
            .authmode = (strlen(DEFAULT_AP_PASS) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    // Set mode to APSTA so station can still reconnect if needed
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    s_ap_active = true;
    s_wifi_state = s_is_connected ? WIFI_STATE_AP_STA_MODE : WIFI_STATE_AP_MODE;

    if (!s_is_connected) {
        strncpy(s_ip_str, "192.168.4.1", sizeof(s_ip_str));
    }
    ESP_LOGI(TAG, "SoftAP active! Connect to SSID '%s' (Pass: '%s'), Web UI: http://192.168.4.1",
             DEFAULT_AP_SSID, DEFAULT_AP_PASS);
}

esp_err_t wifi_manager_init(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_ap_netif = esp_netif_create_default_wifi_ap();
    s_sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(s_ap_netif, "master");
    esp_netif_set_hostname(s_sta_netif, "master");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    return ESP_OK;
}

esp_err_t wifi_manager_start(void) {
    app_settings_t settings;
    config_manager_get(&settings);

    // ALWAYS enable Dual AP+STA mode so SoftAP is NEVER dropped!
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    start_softap();

    // If STA credentials configured, also connect Station
    if (strlen(settings.wifi_ssid) > 0) {
        wifi_config_t wifi_config = {0};
        strncpy((char*)wifi_config.sta.ssid, settings.wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char*)wifi_config.sta.password, settings.wifi_pass, sizeof(wifi_config.sta.password) - 1);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        ESP_LOGI(TAG, "Configuring STA to connect to SSID: '%s' ...", settings.wifi_ssid);
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    } else {
        ESP_LOGW(TAG, "No home WiFi credentials configured yet. SoftAP is active.");
    }

    ESP_ERROR_CHECK(esp_wifi_start());
    init_mdns();

    return ESP_OK;
}

bool wifi_manager_is_connected(void) {
    return s_is_connected;
}

bool wifi_manager_is_ap_active(void) {
    return s_ap_active;
}

void wifi_manager_get_ip_str(char *buf, size_t max_len) {
    if (!buf || max_len == 0) return;
    strncpy(buf, s_ip_str, max_len - 1);
    buf[max_len - 1] = '\0';
}

int8_t wifi_manager_get_rssi(void) {
    if (!s_is_connected) return 0;
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return 0;
}

wifi_state_t wifi_manager_get_state(void) {
    return s_wifi_state;
}

esp_err_t wifi_manager_reconnect(const char *ssid, const char *pass) {
    if (!ssid) return ESP_ERR_INVALID_ARG;

    ESP_LOGI(TAG, "Reconnecting WiFi to SSID: %s (keeping SoftAP active)", ssid);
    s_retry_count = 0;
    s_is_connected = false;

    // Ensure mode remains AP+STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (pass) {
        strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);
    }

    esp_wifi_disconnect();
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    return esp_wifi_connect();
}
