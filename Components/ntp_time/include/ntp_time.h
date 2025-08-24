#ifndef NTP_TIME_H
#define NTP_TIME_H

#include "esp_err.h"

esp_err_t ntp_time_init(void);
char* ntp_get_formatted_time(void);

#endif // NTP_TIME_H