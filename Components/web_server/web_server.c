#include "web_server.h"
#include "cam_reader.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "WEB_SERVER";

// Variables privadas del módulo
static httpd_handle_t server_handle = NULL;
static QueueHandle_t event_queue = NULL;
static TaskHandle_t event_task_handle = NULL;
static SemaphoreHandle_t state_mutex = NULL;
static server_state_t server_state = {0};
static server_config_t server_config = SERVER_DEFAULT_CONFIG();
static bool server_running = false;

// Prototipos de funciones privadas
static void event_processing_task(void *pvParameters);
static void update_server_state(const server_event_t *event);
static esp_err_t setup_http_handlers(void);

// Handlers HTTP
static esp_err_t index_handler(httpd_req_t *req);
static esp_err_t photo_handler(httpd_req_t *req);
static esp_err_t status_handler(httpd_req_t *req);

// Implementación de funciones públicas
esp_err_t web_server_init(void) {
    return web_server_init_with_config(&server_config);
}

esp_err_t web_server_init_with_config(const server_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuración nula");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (server_state.initialized) {
        ESP_LOGW(TAG, "Servidor ya inicializado");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Inicializando servidor web...");
    
    // Copiar configuración
    server_config = *config;
    
    // Crear cola de eventos
    event_queue = xQueueCreate(20, sizeof(server_event_t));
    if (event_queue == NULL) {
        ESP_LOGE(TAG, "Error creando cola de eventos");
        return ESP_ERR_NO_MEM;
    }
    
    // Crear mutex para proteger estado
    state_mutex = xSemaphoreCreateMutex();
    if (state_mutex == NULL) {
        ESP_LOGE(TAG, "Error creando mutex de estado");
        vQueueDelete(event_queue);
        event_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    // Inicializar estado del servidor
    memset(&server_state, 0, sizeof(server_state));
    server_state.initialized = true;
    server_state.last_update_time = esp_timer_get_time();
    
    ESP_LOGI(TAG, "Servidor web inicializado en puerto %d", server_config.port);
    return ESP_OK;
}

esp_err_t web_server_start(void) {
    if (!server_state.initialized) {
        ESP_LOGE(TAG, "Servidor no inicializado");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (server_running) {
        ESP_LOGW(TAG, "Servidor ya ejecutándose");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Iniciando servidor web...");
    
    // Configurar servidor HTTP
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = server_config.port;
    config.max_uri_handlers = server_config.max_uri_handlers;
    config.max_resp_headers = server_config.max_resp_headers;
    
    // Iniciar servidor HTTP
    esp_err_t ret = httpd_start(&server_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando servidor HTTP: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Registrar handlers
    ret = setup_http_handlers();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando handlers HTTP");
        httpd_stop(server_handle);
        server_handle = NULL;
        return ret;
    }
    
    // Crear tarea de procesamiento de eventos
    BaseType_t task_ret = xTaskCreate(
        event_processing_task,
        "web_server_events",
        4096,
        NULL,
        5,
        &event_task_handle
    );
    
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Error creando tarea de eventos");
        httpd_stop(server_handle);
        server_handle = NULL;
        return ESP_FAIL;
    }
    
    server_running = true;
    ESP_LOGI(TAG, "Servidor web iniciado exitosamente");
    return ESP_OK;
}

esp_err_t web_server_stop(void) {
    if (!server_running) {
        ESP_LOGW(TAG, "Servidor no está ejecutándose");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deteniendo servidor web...");
    
    server_running = false;
    
    // Detener tarea de eventos
    if (event_task_handle != NULL) {
        vTaskDelete(event_task_handle);
        event_task_handle = NULL;
    }
    
    // Detener servidor HTTP
    if (server_handle != NULL) {
        esp_err_t ret = httpd_stop(server_handle);
        server_handle = NULL;
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error deteniendo servidor HTTP: %s", esp_err_to_name(ret));
            return ret;
        }
    }
    
    ESP_LOGI(TAG, "Servidor web detenido");
    return ESP_OK;
}

esp_err_t web_server_deinit(void) {
    ESP_LOGI(TAG, "Desinicializando servidor web...");
    
    // Detener servidor si está ejecutándose
    web_server_stop();
    
    // Limpiar recursos
    if (event_queue != NULL) {
        vQueueDelete(event_queue);
        event_queue = NULL;
    }
    
    if (state_mutex != NULL) {
        vSemaphoreDelete(state_mutex);
        state_mutex = NULL;
    }
    
    // Reset estado
    memset(&server_state, 0, sizeof(server_state));
    
    ESP_LOGI(TAG, "Servidor web desinicializado");
    return ESP_OK;
}

QueueHandle_t web_server_get_event_queue(void) {
    return event_queue;
}

server_state_t web_server_get_state(void) {
    server_state_t state_copy = {0};
    
    if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        state_copy = server_state;
        xSemaphoreGive(state_mutex);
    }
    
    return state_copy;
}

bool web_server_is_running(void) {
    return server_running;
}

// Funciones privadas
static void event_processing_task(void *pvParameters) {
    server_event_t event;
    
    ESP_LOGI(TAG, "Tarea de procesamiento de eventos iniciada");
    
    while (server_running) {
        if (xQueueReceive(event_queue, &event, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ESP_LOGD(TAG, "Evento recibido: tipo=%d, timestamp=%llu", event.type, event.timestamp);
            update_server_state(&event);
        }
    }
    
    ESP_LOGI(TAG, "Tarea de procesamiento de eventos terminada");
    vTaskDelete(NULL);
}

static void update_server_state(const server_event_t *event) {
    if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Actualizar timestamp
        server_state.last_update_time = event->timestamp;
        
        switch (event->type) {
            case SERVER_EVENT_DETECTION_STARTED:
                server_state.total_detections = event->detection_data.detection_count;
                server_state.object_currently_detected = event->object_detected;
                server_state.current_sensor_state = event->sensor_state;
                ESP_LOGI(TAG, "Estado actualizado: Nueva detección #%lu", server_state.total_detections);
                break;
                
            case SERVER_EVENT_DETECTION_ENDED:
                server_state.object_currently_detected = event->object_detected;
                server_state.current_sensor_state = event->sensor_state;
                ESP_LOGI(TAG, "Estado actualizado: Detección terminada");
                break;
                
            case SERVER_EVENT_PHOTO_TAKEN:
                server_state.has_photo_available = true;
                ESP_LOGI(TAG, "Estado actualizado: Nueva foto disponible (%zu bytes)", event->photo_data.photo_size);
                break;
                
            default:
                ESP_LOGW(TAG, "Tipo de evento desconocido: %d", event->type);
                break;
        }
        
        xSemaphoreGive(state_mutex);
    } else {
        ESP_LOGE(TAG, "Error obteniendo mutex para actualizar estado");
    }
}

static esp_err_t setup_http_handlers(void) {
    // Handler para página principal
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &index_uri));
    
    // Handler para fotos
    httpd_uri_t photo_uri = {
        .uri = "/photo",
        .method = HTTP_GET,
        .handler = photo_handler,
        .user_ctx = NULL
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &photo_uri));
    
    // Handler para estado JSON
    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server_handle, &status_uri));
    
    ESP_LOGI(TAG, "Handlers HTTP registrados");
    return ESP_OK;
}

// Implementación de handlers HTTP
static esp_err_t index_handler(httpd_req_t *req) {
    server_state_t state = web_server_get_state();
    
    const char* html_template = 
        "<!DOCTYPE html>"
        "<html><head><title>ESP32-CAM Sensor Monitor</title>"
        "<meta charset='utf-8'>"
        "<style>"
        "body { font-family: Arial; text-align: center; margin: 50px; background-color: #f5f5f5; }"
        ".container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }"
        "h1 { color: #333; margin-bottom: 30px; }"
        "img { max-width: 90%%; height: auto; border: 3px solid #333; border-radius: 8px; margin: 20px 0; }"
        ".status { font-size: 18px; margin: 20px 0; padding: 15px; background: #f8f9fa; border-radius: 5px; }"
        ".status p { margin: 10px 0; }"
        ".status strong { color: #2c3e50; }"
        ".detection-count { font-size: 24px; color: #27ae60; font-weight: bold; }"
        ".photo-section { margin-top: 30px; }"
        "</style>"
        "<script>"
        "function autoRefresh() {"
        "  const img = document.getElementById('photo');"
        "  const status = document.getElementById('detectionCount');"
        "  "
        "  // Actualizar foto"
        "  img.src = '/photo?' + new Date().getTime();"
        "  "
        "  // Actualizar estado"
        "  fetch('/status')"
        "    .then(response => response.json())"
        "    .then(data => {"
        "      status.textContent = data.total_detections;"
        "    })"
        "    .catch(error => console.error('Error:', error));"
        "}"
        "setInterval(autoRefresh, 3000);" // Actualiza cada 3 segundos
        "</script>"
        "</head><body>"
        "<div class='container'>"
        "<h1>🔍 Monitor de Sensor E18-D80NK</h1>"
        "<div class='status'>"
        "<p><strong>Detecciones totales:</strong> <span id='detectionCount' class='detection-count'>%lu</span></p>"
        "</div>"
        "<div class='photo-section'>"
        "<h2>📸 Última foto capturada:</h2>"
        "<img id='photo' src='/photo' alt='%s' onerror=\"this.alt='No hay foto disponible'\">"
        "</div>"
        "</div>"
        "</body></html>";
    
    // Preparar respuesta HTML con datos actuales
    char* html_response = malloc(2048);
    if (html_response == NULL) {
        ESP_LOGE(TAG, "Error asignando memoria para HTML");
        return ESP_ERR_NO_MEM;
    }
    
    snprintf(html_response, 2048, html_template, 
             state.total_detections,
             state.has_photo_available ? "Esperando carga de imagen..." : "No hay foto disponible aún");
    
    httpd_resp_set_type(req, "text/html");
    esp_err_t ret = httpd_resp_send(req, html_response, strlen(html_response));
    
    free(html_response);
    return ret;
}

static esp_err_t photo_handler(httpd_req_t *req) {
    uint8_t* photo_buffer = NULL;
    size_t photo_len = 0;
    
    // Obtener datos de la foto desde el componente de cámara
    esp_err_t ret = camera_manager_get_photo_data(&photo_buffer, &photo_len);
    
    if (ret == ESP_OK && photo_buffer != NULL && photo_len > 0) {
        // Configurar headers HTTP para evitar cache
        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
        httpd_resp_set_hdr(req, "Pragma", "no-cache");
        httpd_resp_set_hdr(req, "Expires", "0");
        
        return httpd_resp_send(req, (const char*)photo_buffer, photo_len);
    } else {
        // No hay foto disponible
        const char* no_photo_msg = "No hay foto disponible";
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_set_status(req, "404 Not Found");
        return httpd_resp_send(req, no_photo_msg, strlen(no_photo_msg));
    }
}

static esp_err_t status_handler(httpd_req_t *req) {
    server_state_t state = web_server_get_state();
    
    char status_json[1024];
    snprintf(status_json, sizeof(status_json),
        "{"
        "\"total_detections\":%lu,"
        "\"object_detected\":%s,"
        "\"sensor_state\":%d,"
        "\"has_photo\":%s,"
        "\"last_update\":%llu"
        "}",
        state.total_detections,
        state.object_currently_detected ? "true" : "false",
        state.current_sensor_state,
        state.has_photo_available ? "true" : "false",
        state.last_update_time
    );
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, status_json, strlen(status_json));
}