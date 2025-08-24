#include "ntp_time.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "ntp_time";
static bool ntp_initialized = false;
static char formatted_time_buffer[64];

esp_err_t ntp_time_init(void) {
    if (ntp_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing NTP time");
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
#ifdef CONFIG_NTP_SERVER
    esp_sntp_setservername(0, CONFIG_NTP_SERVER);
#else
    esp_sntp_setservername(0, "pool.ntp.org");
#endif
    esp_sntp_init();

    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (retry >= retry_count) {
        ESP_LOGE(TAG, "Failed to sync time with NTP server");
        return ESP_FAIL;
    }

#ifdef CONFIG_TIMEZONE
    setenv("TZ", CONFIG_TIMEZONE, 1);
#else
    setenv("TZ", "GMT+5", 1);
#endif
    tzset();

    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "NTP time synchronized: %04d-%02d-%02d %02d:%02d:%02d", 
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    ntp_initialized = true;
    return ESP_OK;
}

char* ntp_get_formatted_time(void) {
    if (!ntp_initialized) {
        strcpy(formatted_time_buffer, "No time sync");
        return formatted_time_buffer;
    }

    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);

    const char* am_pm = (timeinfo.tm_hour >= 12) ? "PM" : "AM";
    int hour_12 = timeinfo.tm_hour % 12;
    if (hour_12 == 0) hour_12 = 12;

    snprintf(formatted_time_buffer, sizeof(formatted_time_buffer), 
             "%02d/%02d/%04d %d:%02d %s",
             timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
             hour_12, timeinfo.tm_min, am_pm);

    return formatted_time_buffer;
}