#include "web_server.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "ota_engine.h"

#include <string.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <cJSON.h>

static const char *TAG = "WEB_SERVER";
static httpd_handle_t s_server = NULL;

// Embedded web files (from src/web/)
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t style_css_start[]  asm("_binary_style_css_start");
extern const uint8_t style_css_end[]    asm("_binary_style_css_end");
extern const uint8_t app_js_start[]     asm("_binary_app_js_start");
extern const uint8_t app_js_end[]       asm("_binary_app_js_end");

// Helper: send JSON response
static esp_err_t send_json_response(httpd_req_t *req, cJSON *root) {
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        cJSON_Delete(root);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t res = httpd_resp_send(req, json_str, strlen(json_str));
    free(json_str);
    cJSON_Delete(root);
    return res;
}

// GET / - Serves index.html
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    size_t len = index_html_end - index_html_start;
    return httpd_resp_send(req, (const char *)index_html_start, len);
}

// GET /style.css - Serves CSS
static esp_err_t style_css_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/css");
    size_t len = style_css_end - style_css_start;
    return httpd_resp_send(req, (const char *)style_css_start, len);
}

// GET /app.js - Serves JavaScript
static esp_err_t app_js_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    size_t len = app_js_end - app_js_start;
    return httpd_resp_send(req, (const char *)app_js_start, len);
}

// GET /api/status
static esp_err_t api_status_handler(httpd_req_t *req) {
    app_settings_t cfg;
    config_manager_get(&cfg);

    char ip_buf[32] = {0};
    wifi_manager_get_ip_str(ip_buf, sizeof(ip_buf));

    const esp_partition_t *running = esp_ota_get_running_partition();
    const char *part_label = running ? running->label : "factory";

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_name", cfg.device_name);
    cJSON_AddStringToObject(root, "version", cfg.current_version);
    cJSON_AddStringToObject(root, "hash", cfg.current_hash);
    cJSON_AddStringToObject(root, "partition", part_label);
    cJSON_AddStringToObject(root, "ip", ip_buf);
    cJSON_AddNumberToObject(root, "rssi", wifi_manager_get_rssi());
    cJSON_AddStringToObject(root, "mode", wifi_manager_is_ap_active() ? (wifi_manager_is_connected() ? "AP_STA" : "AP") : "STA");
    cJSON_AddNumberToObject(root, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "uptime", (int64_t)(esp_timer_get_time() / 1000000ULL));

    return send_json_response(req, root);
}

// GET /api/config
static esp_err_t api_config_get_handler(httpd_req_t *req) {
    app_settings_t cfg;
    config_manager_get(&cfg);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_name", cfg.device_name);
    cJSON_AddStringToObject(root, "wifi_ssid", cfg.wifi_ssid);
    cJSON_AddStringToObject(root, "github_url", cfg.github_url);
    cJSON_AddStringToObject(root, "version", cfg.current_version);
    cJSON_AddStringToObject(root, "hash", cfg.current_hash);

    return send_json_response(req, root);
}

// POST /api/config
static esp_err_t api_config_post_handler(httpd_req_t *req) {
    char buf[1024];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    app_settings_t cfg;
    config_manager_get(&cfg);

    cJSON *j_name = cJSON_GetObjectItem(root, "device_name");
    if (j_name && cJSON_IsString(j_name)) {
        strncpy(cfg.device_name, j_name->valuestring, sizeof(cfg.device_name) - 1);
    }

    cJSON *j_ssid = cJSON_GetObjectItem(root, "wifi_ssid");
    if (j_ssid && cJSON_IsString(j_ssid)) {
        strncpy(cfg.wifi_ssid, j_ssid->valuestring, sizeof(cfg.wifi_ssid) - 1);
    }

    cJSON *j_pass = cJSON_GetObjectItem(root, "wifi_pass");
    if (j_pass && cJSON_IsString(j_pass) && strlen(j_pass->valuestring) > 0) {
        strncpy(cfg.wifi_pass, j_pass->valuestring, sizeof(cfg.wifi_pass) - 1);
    }

    cJSON *j_url = cJSON_GetObjectItem(root, "github_url");
    if (j_url && cJSON_IsString(j_url)) {
        strncpy(cfg.github_url, j_url->valuestring, sizeof(cfg.github_url) - 1);
    }

    bool reconnect = false;
    cJSON *j_rec = cJSON_GetObjectItem(root, "reconnect");
    if (j_rec && cJSON_IsTrue(j_rec)) {
        reconnect = true;
    }

    config_manager_save(&cfg);

    cJSON_Delete(root);

    if (reconnect) {
        wifi_manager_reconnect(cfg.wifi_ssid, cfg.wifi_pass);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    return send_json_response(req, resp);
}

// POST /api/ota/check
static esp_err_t api_ota_check_handler(httpd_req_t *req) {
    app_settings_t cfg;
    config_manager_get(&cfg);

    ota_manifest_info_t info = {0};
    esp_err_t err = ota_engine_check(cfg.github_url, &info);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", (err == ESP_OK));
    cJSON_AddStringToObject(root, "remote_version", info.remote_version);
    cJSON_AddStringToObject(root, "remote_hash", info.remote_hash);
    cJSON_AddStringToObject(root, "bin_url", info.bin_url);
    cJSON_AddStringToObject(root, "notes", info.notes);
    cJSON_AddBoolToObject(root, "update_available", info.update_available);
    cJSON_AddStringToObject(root, "message", info.message);

    cJSON *tests_arr = cJSON_CreateArray();
    for (int i = 0; i < info.test_builds_count; i++) {
        cJSON *b = cJSON_CreateObject();
        cJSON_AddStringToObject(b, "name", info.test_builds[i].name);
        cJSON_AddStringToObject(b, "version", info.test_builds[i].version);
        cJSON_AddStringToObject(b, "hash", info.test_builds[i].hash);
        cJSON_AddStringToObject(b, "bin_url", info.test_builds[i].bin_url);
        cJSON_AddStringToObject(b, "notes", info.test_builds[i].notes);
        cJSON_AddItemToArray(tests_arr, b);
    }
    cJSON_AddItemToObject(root, "test_builds", tests_arr);

    return send_json_response(req, root);
}

// POST /api/ota/update
static esp_err_t api_ota_update_handler(httpd_req_t *req) {
    char buf[512] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);

    char bin_url[256] = {0};
    char new_ver[32] = {0};
    char new_hash[65] = {0};

    if (ret > 0) {
        buf[ret] = '\0';
        cJSON *root = cJSON_Parse(buf);
        if (root) {
            cJSON *j_url = cJSON_GetObjectItem(root, "bin_url");
            if (j_url && cJSON_IsString(j_url)) strncpy(bin_url, j_url->valuestring, sizeof(bin_url) - 1);

            cJSON *j_ver = cJSON_GetObjectItem(root, "new_version");
            if (j_ver && cJSON_IsString(j_ver)) strncpy(new_ver, j_ver->valuestring, sizeof(new_ver) - 1);

            cJSON *j_hash = cJSON_GetObjectItem(root, "new_hash");
            if (j_hash && cJSON_IsString(j_hash)) strncpy(new_hash, j_hash->valuestring, sizeof(new_hash) - 1);

            cJSON_Delete(root);
        }
    }

    if (strlen(bin_url) == 0) {
        ota_manifest_info_t last;
        ota_engine_get_last_manifest(&last);
        strncpy(bin_url, last.bin_url, sizeof(bin_url) - 1);
        strncpy(new_ver, last.remote_version, sizeof(new_ver) - 1);
        strncpy(new_hash, last.remote_hash, sizeof(new_hash) - 1);
    }

    if (strlen(bin_url) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No binary URL specified");
        return ESP_FAIL;
    }

    esp_err_t err = ota_engine_start_update(bin_url, new_ver, new_hash);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", (err == ESP_OK));
    cJSON_AddStringToObject(resp, "message", (err == ESP_OK) ? "OTA update initiated" : "Failed to start update task");
    return send_json_response(req, resp);
}

// GET /api/ota/status
static esp_err_t api_ota_status_handler(httpd_req_t *req) {
    ota_progress_t p;
    ota_engine_get_progress(&p);

    const char *state_str = "IDLE";
    switch (p.state) {
        case OTA_STATE_IDLE: state_str = "IDLE"; break;
        case OTA_STATE_CHECKING: state_str = "CHECKING"; break;
        case OTA_STATE_DOWNLOADING: state_str = "DOWNLOADING"; break;
        case OTA_STATE_FLASHING: state_str = "FLASHING"; break;
        case OTA_STATE_SUCCESS: state_str = "SUCCESS"; break;
        case OTA_STATE_FAILED: state_str = "FAILED"; break;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "state", state_str);
    cJSON_AddNumberToObject(root, "progress", p.progress_percent);
    cJSON_AddNumberToObject(root, "written_bytes", p.written_bytes);
    cJSON_AddNumberToObject(root, "total_bytes", p.total_bytes);
    cJSON_AddStringToObject(root, "message", p.message);
    cJSON_AddStringToObject(root, "error", p.error_msg);

    return send_json_response(req, root);
}

// POST /api/ota/upload - Manual binary upload
static esp_err_t api_ota_upload_handler(httpd_req_t *req) {
    size_t total_len = req->content_len;
    ESP_LOGI(TAG, "Manual OTA upload initiated. Total length: %d bytes", total_len);

    esp_err_t err = ota_engine_manual_begin(total_len);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to start manual OTA");
        return ESP_FAIL;
    }

    char *buf = (char *)malloc(1024);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    size_t remaining = total_len;
    while (remaining > 0) {
        int recv_len = httpd_req_recv(req, buf, remaining > 1024 ? 1024 : remaining);
        if (recv_len <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Transfer interrupted");
            return ESP_FAIL;
        }

        err = ota_engine_manual_write((const uint8_t *)buf, recv_len);
        if (err != ESP_OK) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Flash write error");
            return ESP_FAIL;
        }
        remaining -= recv_len;
    }

    free(buf);

    err = ota_engine_manual_end();
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Image validation failed");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Upload complete. Rebooting...");
    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();
    return ESP_OK;
}

// POST /api/restart
static esp_err_t api_restart_handler(httpd_req_t *req) {
    httpd_resp_sendstr(req, "Rebooting ESP32...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

esp_err_t web_server_start(void) {
    if (s_server != NULL) {
        ESP_LOGW(TAG, "Server already running");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting Web Server on port %d...", config.server_port);
    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_root);

    httpd_uri_t uri_style = { .uri = "/style.css", .method = HTTP_GET, .handler = style_css_get_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_style);

    httpd_uri_t uri_app_js = { .uri = "/app.js", .method = HTTP_GET, .handler = app_js_get_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_app_js);

    httpd_uri_t uri_status = { .uri = "/api/status", .method = HTTP_GET, .handler = api_status_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_status);

    httpd_uri_t uri_cfg_get = { .uri = "/api/config", .method = HTTP_GET, .handler = api_config_get_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_cfg_get);

    httpd_uri_t uri_cfg_post = { .uri = "/api/config", .method = HTTP_POST, .handler = api_config_post_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_cfg_post);

    httpd_uri_t uri_ota_chk = { .uri = "/api/ota/check", .method = HTTP_POST, .handler = api_ota_check_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_ota_chk);

    httpd_uri_t uri_ota_upd = { .uri = "/api/ota/update", .method = HTTP_POST, .handler = api_ota_update_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_ota_upd);

    httpd_uri_t uri_ota_stat = { .uri = "/api/ota/status", .method = HTTP_GET, .handler = api_ota_status_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_ota_stat);

    httpd_uri_t uri_ota_upload = { .uri = "/api/ota/upload", .method = HTTP_POST, .handler = api_ota_upload_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_ota_upload);

    httpd_uri_t uri_restart = { .uri = "/api/restart", .method = HTTP_POST, .handler = api_restart_handler, .user_ctx = NULL };
    httpd_register_uri_handler(s_server, &uri_restart);

    ESP_LOGI(TAG, "Web server started and all endpoints registered!");
    return ESP_OK;
}

void web_server_stop(void) {
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
}
