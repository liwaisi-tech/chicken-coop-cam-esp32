#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tipos de eventos que el servidor puede recibir
typedef enum {
    SERVER_EVENT_DETECTION_STARTED,
    SERVER_EVENT_DETECTION_ENDED,
    SERVER_EVENT_PHOTO_TAKEN
} server_event_type_t;

// Estructura de mensaje para la cola del servidor
typedef struct {
    server_event_type_t type;
    uint64_t timestamp;
    
    // Estados del sensor (siempre presentes cuando viene del sensor)
    bool object_detected;
    int sensor_state;
    
    // Datos específicos según el tipo de evento
    union {
        struct {
            uint32_t detection_count;
        } detection_data;
        
        struct {
            size_t photo_size;
            char reason[32];
        } photo_data;
    };
} server_event_t;

// Estado interno del servidor (cache)
typedef struct {
    bool initialized;
    uint32_t total_detections;
    bool object_currently_detected;
    int current_sensor_state;
    bool has_photo_available;
    uint64_t last_update_time;
} server_state_t;

// Configuración del servidor
typedef struct {
    uint16_t port;
    size_t max_uri_handlers;
    size_t max_resp_headers;
    bool enable_cors;
} server_config_t;

#define SERVER_DEFAULT_CONFIG() { \
    .port = 80, \
    .max_uri_handlers = 8, \
    .max_resp_headers = 8, \
    .enable_cors = false \
}

/**
 * @brief Inicializa el servidor web con configuración por defecto
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t web_server_init(void);

/**
 * @brief Inicializa el servidor web con configuración personalizada
 * @param config Configuración del servidor
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t web_server_init_with_config(const server_config_t *config);

/**
 * @brief Inicia el servidor web y la tarea de procesamiento de eventos
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t web_server_start(void);

/**
 * @brief Detiene el servidor web
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t web_server_stop(void);

/**
 * @brief Desinicializa el servidor web y libera recursos
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t web_server_deinit(void);

/**
 * @brief Obtiene el handle de la cola de eventos del servidor
 * @return Handle de la cola o NULL si no está inicializada
 */
QueueHandle_t web_server_get_event_queue(void);

/**
 * @brief Obtiene el estado actual del servidor (cache interno)
 * @return Estructura con el estado actual
 */
server_state_t web_server_get_state(void);

/**
 * @brief Verifica si el servidor está ejecutándose
 * @return true si está ejecutándose, false en caso contrario
 */
bool web_server_is_running(void);

esp_err_t sensor_e18_set_server_queue(QueueHandle_t queue);
#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H
