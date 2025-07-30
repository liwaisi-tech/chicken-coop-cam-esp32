#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sensorE18.h"
#include "cam_reader.h"
#include <inttypes.h>
#include "web_server.h"

#define DEBOUNCE_TIME_MS 50
#define PERIODIC_PHOTO_INTERVAL_US 2000000  // 2 segundos en microsegundos

static const char* TAG = "E18-D80NK";

// Variables privadas del módulo
static QueueHandle_t sensor_event_queue = NULL;
static QueueHandle_t server_queue = NULL;
static sensor_e18_config_t current_config = SENSOR_E18_DEFAULT_CONFIG;
static sensor_statistics_t sensor_stats = {0};
static esp_timer_handle_t periodic_photo_timer = NULL;

// Prototipos de funciones privadas
static esp_err_t send_server_event(server_event_type_t type, const char* reason);

// Función de interrupción
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(sensor_event_queue, &gpio_num, &xHigherPriorityTaskWoken);
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// Callback del timer para fotos periódicas
static void periodic_photo_callback(void* arg) {
    // Solo tomar foto si el objeto sigue detectado
    int sensor_state = gpio_get_level(current_config.pin);
    
    if (sensor_state == 1 && sensor_stats.object_detected) {
        ESP_LOGI(TAG, "📸 Foto periódica - objeto permanece presente");
        camera_manager_take_photo("objeto permanece presente");
        
        // Notificar foto al servidor
        send_server_event(SERVER_EVENT_PHOTO_TAKEN, "objeto permanece presente");
    }
}

// Función para enviar eventos al servidor
static esp_err_t send_server_event(server_event_type_t type, const char* reason) {
    if (server_queue == NULL) {
        // Cola no configurada, continuar sin enviar
        return ESP_OK;
    }
    
    server_event_t event = {
        .type = type,
        .timestamp = esp_timer_get_time(),
        .object_detected = sensor_stats.object_detected,
        .sensor_state = gpio_get_level(current_config.pin)
    };
    
    switch (type) {
        case SERVER_EVENT_DETECTION_STARTED:
        case SERVER_EVENT_DETECTION_ENDED:
            event.detection_data.detection_count = sensor_stats.detection_count;
            break;
            
        case SERVER_EVENT_PHOTO_TAKEN:
            // Los datos de la foto los maneja el componente de cámara
            if (reason) {
                strncpy(event.photo_data.reason, reason, sizeof(event.photo_data.reason) - 1);
                event.photo_data.reason[sizeof(event.photo_data.reason) - 1] = '\0';
            }
            break;
    }
    
    BaseType_t result = xQueueSend(server_queue, &event, pdMS_TO_TICKS(100));
    if (result != pdTRUE) {
        ESP_LOGW(TAG, "No se pudo enviar evento al servidor web");
        return ESP_ERR_TIMEOUT;
    }
    
    ESP_LOGD(TAG, "Evento enviado al servidor: tipo=%d", type);
    return ESP_OK;
}

// Tarea principal de detección
esp_err_t sensor_e18_set_server_queue(QueueHandle_t queue) {
    server_queue = queue;
    ESP_LOGI(TAG, "Cola del servidor web configurada");
    return ESP_OK;
}

// Tarea de detección para enviar eventos
static void sensor_detection_task(void *pvParameter) {
    uint32_t io_num;
    
    ESP_LOGI(TAG, "Tarea de detección iniciada");
    
    while(1) {
        if(xQueueReceive(sensor_event_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Evento recibido en GPIO %" PRIu32, io_num);
            
            // Debouncing
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
            
            int sensor_state = gpio_get_level(current_config.pin);
            ESP_LOGI(TAG, "Estado del sensor: %d", sensor_state);
            
            if (sensor_state == 1) {
                // OBJETO DETECTADO
                if (!sensor_stats.object_detected) {
                    // Primera detección
                    sensor_stats.object_detected = true;
                    sensor_stats.detection_count++;
                    sensor_stats.last_detection_time = esp_timer_get_time();
                    
                    ESP_LOGI(TAG, "✅ NUEVO OBJETO DETECTADO #%" PRIu32, sensor_stats.detection_count);
                    
                    // Enviar evento al servidor
                    send_server_event(SERVER_EVENT_DETECTION_STARTED, NULL);
                    
                    // Tomar foto inmediata
                    camera_manager_take_photo("detección inicial");
                    
                    // Notificar foto al servidor
                    send_server_event(SERVER_EVENT_PHOTO_TAKEN, "detección inicial");
                    
                    // Iniciar timer para fotos periódicas
                    if (periodic_photo_timer == NULL) {
                        esp_timer_create_args_t timer_args = {
                            .callback = periodic_photo_callback,
                            .name = "periodic_photo_timer"
                        };
                        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &periodic_photo_timer));
                    }
                    
                    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_photo_timer, PERIODIC_PHOTO_INTERVAL_US));
                    ESP_LOGI(TAG, "⏰ Timer de fotos periódicas iniciado");
                }
                
            } else {
                // OBJETO NO DETECTADO
                if (sensor_stats.object_detected) {
                    sensor_stats.object_detected = false;
                    ESP_LOGI(TAG, "❌ Objeto retirado - Total: %" PRIu32, sensor_stats.detection_count);
                    
                    // Enviar evento al servidor
                    send_server_event(SERVER_EVENT_DETECTION_ENDED, NULL);
                    
                    // Detener timer de fotos periódicas
                    if (periodic_photo_timer != NULL) {
                        esp_timer_stop(periodic_photo_timer);
                        ESP_LOGI(TAG, "⏰ Timer de fotos detenido");
                    }
                }
            }
        }
    }
}

// Funciones públicas
esp_err_t sensor_e18_init(void) {
    return sensor_e18_init_with_config(&current_config);
}

esp_err_t sensor_e18_init_with_config(const sensor_e18_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuración nula");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copiar configuración
    current_config = *config;
    
    ESP_LOGI(TAG, "Inicializando en GPIO %d", current_config.pin);
    
    // Crear cola de eventos
    if (sensor_event_queue == NULL) {
        sensor_event_queue = xQueueCreate(10, sizeof(uint32_t));
        if (sensor_event_queue == NULL) {
            ESP_LOGE(TAG, "Error creando cola");
            return ESP_ERR_NO_MEM;
        }
    }
    
    // Configurar GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << current_config.pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = current_config.pull_up_en,
        .pull_down_en = current_config.pull_down_en,
        .intr_type = current_config.intr_type
    };
    
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    // Instalar servicio de interrupciones
    esp_err_t isr_result = gpio_install_isr_service(0);
    if (isr_result == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "Servicio ISR ya instalado");
    } else {
        ESP_ERROR_CHECK(isr_result);
    }
    
    // Registrar handler de interrupción
    ESP_ERROR_CHECK(gpio_isr_handler_add(current_config.pin, gpio_isr_handler, (void*) current_config.pin));
    
    ESP_LOGI(TAG, "Sensor inicializado correctamente");
    return ESP_OK;
}

esp_err_t sensor_e18_start_detection_task(void) {
    BaseType_t result = xTaskCreate(
        sensor_detection_task, 
        "sensor_detection", 
        4096, 
        NULL, 
        10, 
        NULL
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Error creando tarea de detección");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Tarea de detección creada");
    return ESP_OK;
}

sensor_statistics_t sensor_e18_get_statistics(void) {
    return sensor_stats;
}

int sensor_e18_read_state(void) {
    return gpio_get_level(current_config.pin);
}

sensor_e18_config_t sensor_e18_get_config(void) {
    return current_config;
}

esp_err_t sensor_e18_deinit(void) {
    // Detener timer si existe
    if (periodic_photo_timer != NULL) {
        esp_timer_stop(periodic_photo_timer);
        esp_timer_delete(periodic_photo_timer);
        periodic_photo_timer = NULL;
    }
    
    // Remover handler de interrupción
    if (current_config.pin >= 0) {
        gpio_isr_handler_remove(current_config.pin);
    }
    
    // Eliminar cola
    if (sensor_event_queue != NULL) {
        vQueueDelete(sensor_event_queue);
        sensor_event_queue = NULL;
    }
    
    // Reset estadísticas
    memset(&sensor_stats, 0, sizeof(sensor_stats));
    
    ESP_LOGI(TAG, "Sensor desinicializado");
    return ESP_OK;
}
