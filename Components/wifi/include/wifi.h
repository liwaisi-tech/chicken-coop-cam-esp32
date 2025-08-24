#include "esp_err.h"
#ifndef WIFI_H
#define WIFI_H

esp_err_t wifi_init_sta(void);
char* wifi_get_local_ip(void);

#endif // WIFI_H