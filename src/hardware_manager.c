#include "hardware_manager.h"
#include <driver/gpio.h>
#include <esp_log.h>

static const char *TAG = "HW_MGR";

// ESP32-C3 SuperMini onboard blue LED is active LOW: 0 = ON, 1 = OFF
static bool s_led_state = false;

esp_err_t hardware_manager_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ONBOARD_LED_GPIO),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure onboard LED GPIO: %s", esp_err_to_name(err));
        return err;
    }

    // Default off (active-low means HIGH is off)
    s_led_state = false;
    gpio_set_level(ONBOARD_LED_GPIO, 1);
    ESP_LOGI(TAG, "Hardware manager initialized. Onboard LED configured on GPIO %d", ONBOARD_LED_GPIO);
    return ESP_OK;
}

esp_err_t hardware_set_led(bool state) {
    s_led_state = state;
    // Active low on ESP32-C3 SuperMini
    esp_err_t err = gpio_set_level(ONBOARD_LED_GPIO, state ? 0 : 1);
    ESP_LOGI(TAG, "Onboard LED turned %s", state ? "ON" : "OFF");
    return err;
}

bool hardware_get_led(void) {
    return s_led_state;
}

esp_err_t hardware_toggle_led(void) {
    return hardware_set_led(!s_led_state);
}

esp_err_t hardware_set_gpio(int gpio_num, int level) {
    if (gpio_num < 0 || gpio_num > 21) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT);
    return gpio_set_level(gpio_num, level ? 1 : 0);
}

int hardware_get_gpio(int gpio_num) {
    if (gpio_num < 0 || gpio_num > 21) {
        return -1;
    }
    return gpio_get_level(gpio_num);
}
