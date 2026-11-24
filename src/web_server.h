#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the HTTP web server on port 80.
 */
esp_err_t web_server_start(void);

/**
 * @brief Stop the HTTP web server.
 */
void web_server_stop(void);

#ifdef __cplusplus
}
#endif
