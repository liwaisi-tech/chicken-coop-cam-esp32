#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sensorE18.h"
#include <inttypes.h>

#define DEBOUNCE_TIME_MS 50

static const char* TAG = "E18-D80NK";
static QueueHandle_t sensor_event_queue = NULL;
static sensor_e18_config_t current_config = SENSOR_E18_DEFAULT_CONFIG;

// Función de interrupción - SE EJECUTA AUTOMÁTICAMENTE cuando hay cambio de flanco
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    // Envía el evento a la cola para procesarlo en la tarea principal
    xQueueSendFromISR(sensor_event_queue, &gpio_num, NULL);
}

// Tarea que procesa los eventos del sensor
void sensor_task(void *pvParameter) {
    uint32_t io_num;
    static uint32_t detection_count = 0;
    
    ESP_LOGI(TAG, "Tarea del sensor iniciada");
    
    while(1) {
        // Espera por eventos de la cola (se bloquea hasta recibir algo)
        if(xQueueReceive(sensor_event_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Interrupción recibida en GPIO %" PRIu32, io_num);
            
            // Debouncing - espera para evitar lecturas múltiples
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
            
            // Lee el estado actual del sensor
            int sensor_state = gpio_get_level(current_config.pin);
            
            ESP_LOGI(TAG, "Sensor_state: %d", sensor_state);
            
            //procesa  el cambió estado 
            if (sensor_state == 1) {
                // Sensor HIGH = objeto detectado
                detection_count++;
                ESP_LOGI(TAG, "✅ OBJETO DETECTADO - Contador: %" PRIu32, detection_count);         
            }
            else{
            // Sensor LOW = sin objeto
            ESP_LOGI(TAG, "❌ Sin objeto - Contador actual: %" PRIu32, detection_count);
            }
        }
        
    }

}

// Función de inicialización con valores por defecto
void init_sensor(void) {
    init_sensor_with_config(&current_config);
}

// Función de inicialización con configuración personalizada
void init_sensor_with_config(const sensor_e18_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Error: Configuración nula");
        return;
    }
    
    // Copiar la configuración
    current_config = *config;
    
    ESP_LOGI(TAG, "Inicializando sensor E18-D80NK en GPIO %d", current_config.pin);
    
    // 1. Crear la cola para comunicación entre interrupción y tarea
    sensor_event_queue = xQueueCreate(10, sizeof(uint32_t));
    if (sensor_event_queue == NULL) {
        ESP_LOGE(TAG, "Error: No se pudo crear la cola");
        return;
    }
    
    // 2. Configurar el GPIO usando la estructura
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << current_config.pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = current_config.pull_up_en,
        .pull_down_en = current_config.pull_down_en,
        .intr_type = current_config.intr_type
    };
    
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_LOGI(TAG, "GPIO configurado correctamente");
    
    // 3. Instalar el servicio de interrupciones GPIO (solo una vez)
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    
    // 4. AQUÍ SE REGISTRA LA FUNCIÓN DE INTERRUPCIÓN
    ESP_ERROR_CHECK(gpio_isr_handler_add(current_config.pin, gpio_isr_handler, (void*) current_config.pin));
    
    ESP_LOGI(TAG, "Handler de interrupción registrado");
    
    // 5. Crear la tarea que procesa los eventos
    xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 10, NULL);
    ESP_LOGI(TAG, "Tarea del sensor creada");
    
    ESP_LOGI(TAG, "✅ Sensor inicializado correctamente");
}