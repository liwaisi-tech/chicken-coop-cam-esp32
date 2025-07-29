#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "sensorE18.h"

static const char* TAG = "MAIN";

// Función principal de la aplicación
void app_main(void) {
    ESP_LOGI(TAG, "=== INICIANDO APLICACIÓN E18-D80NK ===");
    // Inicializar el sensor
    init_sensor();
    
    // El main puede hacer otras cosas aquí
    while(1) {
        ESP_LOGI(TAG, "Aplicación ejecutándose... esperando detecciones");
        vTaskDelay(pdMS_TO_TICKS(3000)); // Espera 5 segundos
    }
}