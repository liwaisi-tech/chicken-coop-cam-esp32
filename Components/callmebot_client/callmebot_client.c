#include "callmebot_client.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
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

    // URL encode el timestamp (reemplazar espacios con +)
    char encoded_timestamp[64];
    int j = 0;
    for (int i = 0; timestamp[i] != '\0' && j < sizeof(encoded_timestamp) - 1; i++) {
        if (timestamp[i] == ' ') {
            encoded_timestamp[j++] = '+';
        } else if (timestamp[i] == ':') {
            encoded_timestamp[j++] = '%';
            encoded_timestamp[j++] = '3';
            encoded_timestamp[j++] = 'A';
        } else if (timestamp[i] == '/') {
            encoded_timestamp[j++] = '%';
            encoded_timestamp[j++] = '2';
            encoded_timestamp[j++] = 'F';
        } else {
            encoded_timestamp[j++] = timestamp[i];
        }
    }
    encoded_timestamp[j] = '\0';
    
    // Simplificar mensaje para evitar problemas de encoding
    char message[256];
    snprintf(message, sizeof(message), 
             "Movimiento+detectado+%s+%s",
             encoded_timestamp, server_url);

    // URL encode del número de teléfono (+ se convierte en %2B)
    char encoded_phone[32];
    if (CONFIG_CALLMEBOT_PHONE_NUMBER[0] == '+') {
        snprintf(encoded_phone, sizeof(encoded_phone), "%%2B%s", &CONFIG_CALLMEBOT_PHONE_NUMBER[1]);
    } else {
        strncpy(encoded_phone, CONFIG_CALLMEBOT_PHONE_NUMBER, sizeof(encoded_phone) - 1);
    }
    
    char url[1024];
    snprintf(url, sizeof(url),
             "https://api.callmebot.com/whatsapp.php?phone=%s&text=%s&apikey=%s",
             encoded_phone, message, CONFIG_CALLMEBOT_API_KEY);
    
    ESP_LOGI(TAG, "📱 CallMeBot URL: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .timeout_ms = 15000,
        .user_agent = "ESP32-CallMeBot/1.0",
        .crt_bundle_attach = esp_crt_bundle_attach,
        .disable_auto_redirect = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    // Configurar opciones adicionales después de la inicialización
    esp_http_client_set_header(client, "Connection", "close");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "📱 CallMeBot Response - Status: %d, Length: %d", status_code, content_length);
        
        if (status_code == 200) {
            ESP_LOGI(TAG, "✅ WhatsApp message sent successfully!");
            err = ESP_OK;
        } else if (status_code >= 400) {
            ESP_LOGE(TAG, "❌ CallMeBot API error - Status: %d", status_code);
            err = ESP_FAIL;
        } else {
            ESP_LOGW(TAG, "⚠️ CallMeBot unexpected status: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "❌ HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}