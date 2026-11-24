#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ONBOARD_LED_GPIO 8

/**
 * @brief Initialize hardware peripherals (GPIOs, onboard LED)
 */
esp_err_t hardware_manager_init(void);

/**
 * @brief Set the onboard LED state (true = ON, false = OFF)
 */
esp_err_t hardware_set_led(bool state);

/**
 * @brief Get the current onboard LED state
 */
bool hardware_get_led(void);

/**
 * @brief Toggle the onboard LED state
 */
esp_err_t hardware_toggle_led(void);

/**
 * @brief Set level of arbitrary output GPIO pin
 */
esp_err_t hardware_set_gpio(int gpio_num, int level);

/**
 * @brief Get level of arbitrary GPIO pin
 */
int hardware_get_gpio(int gpio_num);

#ifdef __cplusplus
}
#endif
