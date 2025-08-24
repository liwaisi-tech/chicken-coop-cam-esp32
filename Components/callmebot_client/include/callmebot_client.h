#ifndef CALLMEBOT_CLIENT_H
#define CALLMEBOT_CLIENT_H

#include "esp_err.h"

esp_err_t callmebot_init(void);
esp_err_t callmebot_send_detection_alert(const char* timestamp, const char* server_url);

#endif // CALLMEBOT_CLIENT_H