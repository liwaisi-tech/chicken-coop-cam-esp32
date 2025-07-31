//Coordinación principal del sistema
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "sensorE18.h"
#include "cam_reader.h"
#include "web_server.h"

// Declaración externa de la función WiFi
extern esp_err_t wifi_init_sta(void);

static const char *TAG = "MAIN_SYSTEM";

void app_main(void) {
    ESP_LOGI(TAG, "=== INICIANDO SISTEMA INTEGRADO SENSOR + CÁMARA ===");
    
    // 1. Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "✅ NVS inicializado");
    
    // 2. Inicializar cámara
    ESP_LOGI(TAG, "Inicializando módulo de cámara...");
    if (camera_manager_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando cámara");
        return;
    }
    ESP_LOGI(TAG, "✅ Cámara inicializada");
    
    // Esperar un momento para que la cámara se estabilice
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Detectar automáticamente condiciones de luz y optimizar
    ESP_LOGI(TAG, "🔧 Detectando condiciones de luz y optimizando...");
    camera_manager_auto_optimize_lighting();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 3. Inicializar WiFi
    ESP_LOGI(TAG, "Conectando a WiFi...");
    if (wifi_init_sta() != ESP_OK) {
        ESP_LOGE(TAG, " Error conectando WiFi");
        return;
    }
    ESP_LOGI(TAG, "✅ WiFi conectado");
    
    // 4. Inicializar sensor E18-D80NK
    ESP_LOGI(TAG, "Inicializando sensor E18-D80NK...");
    if (sensor_e18_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando sensor");
        return;
    }
    ESP_LOGI(TAG, "✅ Sensor inicializado");
    
    // Test del sensor para diagnóstico
    ESP_LOGI(TAG, "Ejecutando test del sensor...");
    sensor_e18_test();
    
    // 5. Inicializar servidor web
    ESP_LOGI(TAG, "Iniciando servidor web...");
    if (web_server_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando servidor web");
        return;
    }
    ESP_LOGI(TAG, "✅ Servidor web inicializado");
    
    // Iniciar servidor web
    if (web_server_start() != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando servidor web");
        return;
    }
    ESP_LOGI(TAG, "✅ Servidor web andando");
    
    // 6. Iniciar la lógica de integración sensor-cámara
    ESP_LOGI(TAG, "Iniciando lógica de integración...");
    if (sensor_e18_start_detection_task() != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando detección");
        return;
    }
    ESP_LOGI(TAG, "✅ Sistema de detección iniciado");
    
    ESP_LOGI(TAG, "🎉 SISTEMA COMPLETAMENTE INICIALIZADO");
    ESP_LOGI(TAG, "🌐 Accede a la interfaz web desde tu navegador con la IP del ESP32");
    
    // Esperar 5 segundos para que todo se estabilice
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // FASE DE PRUEBAS: Simular detecciones para validar el sistema
    ESP_LOGI(TAG, "🧪 INICIANDO FASE DE PRUEBAS CON SIMULACIONES");
    
    // Simular detección de objeto
    ESP_LOGI(TAG, "🧪 Test 1: Simulando detección de objeto...");
    sensor_e18_simulate_detection(true);
    vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar 3 segundos
    
    // Simular retiro de objeto
    ESP_LOGI(TAG, "🧪 Test 2: Simulando retiro de objeto...");
    sensor_e18_simulate_detection(false);
    vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar 3 segundos
    
    ESP_LOGI(TAG, "🧪 FASE DE PRUEBAS COMPLETADA - Iniciando monitoreo normal");
    
    // Bucle principal - monitoreo con lecturas periódicas del sensor
    while(1) {
        sensor_statistics_t stats = sensor_e18_get_statistics();
        int current_sensor_state = sensor_e18_read_state();
        
        ESP_LOGI(TAG, "📊 Estado: Detecciones=%lu | Activo=%s | Fotos=%lu | Pin=%d", 
                stats.detection_count, 
                stats.object_detected ? "SÍ" : "NO",
                camera_manager_get_photo_count(),
                current_sensor_state);
        vTaskDelay(pdMS_TO_TICKS(10000)); // Log cada 10 segundos
    }
}