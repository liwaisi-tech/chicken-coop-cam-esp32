#include "callmebot_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "callmebot_client";

#ifndef CONFIG_CALLMEBOT_PHONE_NUMBER
#define CONFIG_CALLMEBOT_PHONE_NUMBER "+57XXXXXXXXX"
#endif

#ifndef CONFIG_CALLMEBOT_API_KEY
#define CONFIG_CALLMEBOT_API_KEY "XXXXXX"
#endif

esp_err_t callmebot_init(void) {
    ESP_LOGI(TAG, "CallMeBot client initialized");
    return ESP_OK;
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

esp_err_t callmebot_send_detection_alert(const char* timestamp, const char* server_url) {
    if (!timestamp || !server_url) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    char message[512];
    snprintf(message, sizeof(message), 
             "Movimiento+detectado+en+gallinero+%s+Ver+imagen:+%s",
             timestamp, server_url);

    char url[1024];
    snprintf(url, sizeof(url),
             "https://api.callmebot.com/whatsapp.php?phone=%s&text=%s&apikey=%s",
             CONFIG_CALLMEBOT_PHONE_NUMBER, message, CONFIG_CALLMEBOT_API_KEY);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "WhatsApp message sent, status: %d", status_code);
        if (status_code == 200) {
            err = ESP_OK;
        } else {
            ESP_LOGW(TAG, "CallMeBot returned non-200 status: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}