#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_ota_ops.h"
#include "driver/gpio.h"

#include "app_config.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "hardware_manager.h"
#include "ota_engine.h"
#include "web_server.h"

static const char *TAG = "MASTER_HELPER";

// Standard onboard Status LED on ESP32-C3 (e.g. SuperMini blue LED on GPIO 8)
#define STATUS_LED_GPIO GPIO_NUM_8

static void status_led_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    // Active LOW drives the onboard blue LED on ESP32-C3 SuperMini
    gpio_set_level(STATUS_LED_GPIO, 0);
    ESP_LOGI(TAG, "Hardware Status LED (GPIO %d) set to ON (Blue Glow)", STATUS_LED_GPIO);
}

void app_main(void) {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "        MASTER HELPER - ESP32-C3 CORE             ");
    ESP_LOGI(TAG, "==================================================");

    // Initialize onboard status LED immediately for physical upload feedback
    status_led_init();

    // Print chip & flash diagnostics
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    ESP_LOGI(TAG, "Model: ESP32-C3 rev %d, Cores: %d, Features: %s%s",
             chip_info.revision,
             chip_info.cores,
             (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi " : "",
             (chip_info.features & CHIP_FEATURE_BT) ? "BT/BLE " : "");
    ESP_LOGI(TAG, "Flash size: %lu MB", (unsigned long)(flash_size / (1024 * 1024)));

    // Active OTA Partition info
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running) {
        ESP_LOGI(TAG, "Active Running Partition: %s (Type 0x%02x, Subtype 0x%02x, Offset 0x%08lx)",
                 running->label, running->type, running->subtype, running->address);
    }

    // 1. Initialize Persistent Config Storage (NVS)
    ESP_ERROR_CHECK(config_manager_init());

    // 1b. Initialize Hardware Peripherals (Onboard LED on GPIO 8)
    ESP_ERROR_CHECK(hardware_manager_init());

    app_settings_t settings;
    config_manager_get(&settings);
    ESP_LOGI(TAG, "Firmware Version: %s | Hash: %s", settings.current_version, settings.current_hash);
    ESP_LOGI(TAG, "Device Name: %s", settings.device_name);

    // 2. Initialize OTA Update Engine
    ota_engine_init();

    // 3. Initialize & Start Wi-Fi Manager (STA with SoftAP fallback)
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(wifi_manager_start());

    // 4. Start HTTP Web Server and REST API
    ESP_ERROR_CHECK(web_server_start());

    ESP_LOGI(TAG, "--------------------------------------------------");
    ESP_LOGI(TAG, "Master Helper Web UI is active and listening!");
    ESP_LOGI(TAG, "In SoftAP mode: Connect to '%s' (Pass: '%s')", DEFAULT_AP_SSID, DEFAULT_AP_PASS);
    ESP_LOGI(TAG, "Web UI Address: http://192.168.4.1 (or assigned STA IP)");
    ESP_LOGI(TAG, "--------------------------------------------------");

    // Main supervisory loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        char ip[32] = {0};
        wifi_manager_get_ip_str(ip, sizeof(ip));
        ESP_LOGD(TAG, "Heartbeat | IP: %s | Free Heap: %lu bytes",
                 ip, (unsigned long)esp_get_free_heap_size());
    }
}
