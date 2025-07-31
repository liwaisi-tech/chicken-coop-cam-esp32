// cam_reader.c - Responsabilidad única: gestión de la cámara
#include "cam_reader.h"
#include "web_server.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
#include <string.h>

// Configuración de pines del ESP32-CAM
#define CAM_PIN_PWDN    32 
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22
#define CONFIG_XCLK_FREQ 10000000  // Reducir frecuencia para mejor estabilidad

static const char *TAG = "CAMERA_MANAGER";

// Variables privadas del módulo
static camera_fb_t *current_photo = NULL;
static SemaphoreHandle_t photo_mutex = NULL;
static camera_info_t camera_info = {0};
static QueueHandle_t server_queue = NULL;

// Función privada para enviar eventos al servidor
static esp_err_t send_server_event(const char* reason, size_t photo_size) {
    if (server_queue == NULL) {
        // Cola no configurada, continuar sin enviar
        return ESP_OK;
    }
    
    server_event_t event = {
        .type = SERVER_EVENT_PHOTO_TAKEN,
        .timestamp = esp_timer_get_time(),
        .object_detected = false,  // Este campo lo maneja el sensor
        .sensor_state = -1         // Este campo lo maneja el sensor
    };
    
    // Configurar datos específicos de la foto
    event.photo_data.photo_size = photo_size;
    if (reason) {
        strncpy(event.photo_data.reason, reason, sizeof(event.photo_data.reason) - 1);
        event.photo_data.reason[sizeof(event.photo_data.reason) - 1] = '\0';
    }
    
    BaseType_t result = xQueueSend(server_queue, &event, pdMS_TO_TICKS(100));
    if (result != pdTRUE) {
        ESP_LOGW(TAG, "No se pudo enviar evento de foto al servidor web");
        return ESP_ERR_TIMEOUT;
    }
    
    ESP_LOGD(TAG, "Evento de foto enviado al servidor: %zu bytes", photo_size);
    return ESP_OK;
}

esp_err_t camera_manager_init(void) {
    camera_config_custom_t default_config = CAMERA_DEFAULT_CONFIG();
    return camera_manager_init_with_config(&default_config);
}

esp_err_t camera_manager_init_with_config(const camera_config_custom_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuración nula");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Inicializando cámara...");
    
    // Crear mutex para proteger acceso a foto
    photo_mutex = xSemaphoreCreateMutex();
    if (photo_mutex == NULL) {
        ESP_LOGE(TAG, "Error creando mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Configuración de la cámara
    camera_config_t camera_config = {
        .pin_pwdn  = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        
        .xclk_freq_hz = CONFIG_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        
        .pixel_format = config->pixel_format,
        .frame_size = config->frame_size,
        .jpeg_quality = config->jpeg_quality,
        .fb_count = config->fb_count,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY
    };
    
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Error inicializando cámara: %s", esp_err_to_name(err));
        vSemaphoreDelete(photo_mutex);
        photo_mutex = NULL;
        return err;
    }
    
    // Verificar que el sensor funciona correctamente
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        ESP_LOGE(TAG, "❌ No se pudo obtener el sensor de la cámara");
        esp_camera_deinit();
        vSemaphoreDelete(photo_mutex);
        photo_mutex = NULL;
        return ESP_FAIL;
    }
    
    // Configurar ajustes optimizados para máxima iluminación
    s->set_brightness(s, 2);     // Brillo máximo: +2
    s->set_contrast(s, 2);       // Contraste máximo: +2  
    s->set_saturation(s, 1);     // Saturación aumentada: +1
    s->set_whitebal(s, 1);       // Balance de blancos automático
    s->set_awb_gain(s, 1);       // Ganancia AWB automática
    s->set_wb_mode(s, 0);        // Modo WB automático
    s->set_exposure_ctrl(s, 1);  // Control de exposición automático
    s->set_aec2(s, 1);           // AEC2 habilitado para mejor exposición
    s->set_ae_level(s, 2);       // Nivel AE máximo: +2 (más exposición)
    s->set_aec_value(s, 1200);   // Valor AEC máximo: 1200 (máximo tiempo exposición)
    s->set_gain_ctrl(s, 1);      // Control de ganancia automático
    s->set_agc_gain(s, 30);      // Ganancia AGC máxima: 25
    s->set_gainceiling(s, (gainceiling_t)6); // Techo de ganancia máximo (128x)
    s->set_bpc(s, 0);            // BPC deshabilitado
    s->set_wpc(s, 1);            // WPC habilitado
    s->set_raw_gma(s, 1);        // Gamma raw habilitado
    s->set_lenc(s, 1);           // Corrección de lente habilitada
    s->set_hmirror(s, 0);        // Sin espejo horizontal
    s->set_vflip(s, 0);          // Sin volteo vertical
    s->set_dcw(s, 1);            // DCW habilitado
    s->set_colorbar(s, 0);       // Sin barra de colores
    
    // Inicializar información de la cámara
    camera_info.initialized = true;
    camera_info.frame_size = config->frame_size;
    camera_info.pixel_format = config->pixel_format;
    camera_info.jpeg_quality = config->jpeg_quality;
    camera_info.photo_count = 0;
    camera_info.last_photo_size = 0;
    camera_info.last_photo_time = 0;
    
    ESP_LOGI(TAG, "✅ Cámara inicializada correctamente");
    ESP_LOGI(TAG, "📋 Configuración: 1280x720 (720p HD), JPEG calidad %d, %d buffers", 
             config->jpeg_quality, config->fb_count);
    
    return ESP_OK;
}

esp_err_t camera_manager_take_photo(const char* reason) {
    if (!camera_info.initialized) {
        ESP_LOGE(TAG, "Cámara no inicializada");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "📸 Tomando foto por: %s", reason ? reason : "razón no especificada");
    
    camera_fb_t *new_photo = NULL;
    int retries = 3;
    
    // Reintentar hasta 3 veces si falla
    for (int i = 0; i < retries; i++) {
        new_photo = esp_camera_fb_get();
        if (new_photo) {
            ESP_LOGI(TAG, "✅ Foto capturada exitosamente en intento %d", i + 1);
            break;
        }
        
        ESP_LOGW(TAG, "⚠️ Intento %d/%d falló, reintentando...", i + 1, retries);
        vTaskDelay(pdMS_TO_TICKS(100)); // Esperar 100ms antes del siguiente intento
    }
    
    if (!new_photo) {
        ESP_LOGE(TAG, "❌ Error capturando foto después de %d intentos", retries);
        return ESP_FAIL;
    }
    
    // Proteger acceso concurrente
    if (xSemaphoreTake(photo_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Liberar foto anterior si existe
        if (current_photo) {
            esp_camera_fb_return(current_photo);
            ESP_LOGD(TAG, "Foto anterior liberada");
        }
        
        // Almacenar nueva foto
        current_photo = new_photo;
        camera_info.photo_count++;
        camera_info.last_photo_size = current_photo->len;
        camera_info.last_photo_time = esp_timer_get_time();
        
        ESP_LOGI(TAG, "📷 Nueva foto #%lu almacenada - Tamaño: %zu bytes (%.1f KB)", 
                camera_info.photo_count, current_photo->len, current_photo->len / 1024.0);
        
        // Notificar al servidor web (fuera del mutex crítico)
        size_t photo_size = current_photo->len;
        xSemaphoreGive(photo_mutex);
        
        // Enviar evento al servidor
        send_server_event(reason, photo_size);
        
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error obteniendo mutex");
        esp_camera_fb_return(new_photo);
        return ESP_ERR_TIMEOUT;
    }
}

camera_fb_t* camera_manager_get_current_photo(void) {
    camera_fb_t* photo_copy = NULL;
    
    if (xSemaphoreTake(photo_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        photo_copy = current_photo;
        xSemaphoreGive(photo_mutex);
    }
    
    return photo_copy;
}

esp_err_t camera_manager_get_photo_data(uint8_t** buffer, size_t* len) {
    if (!buffer || !len) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *buffer = NULL;
    *len = 0;
    
    if (xSemaphoreTake(photo_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (current_photo) {
            *buffer = current_photo->buf;
            *len = current_photo->len;
            xSemaphoreGive(photo_mutex);
            return ESP_OK;
        }
        xSemaphoreGive(photo_mutex);
    }
    
    return ESP_ERR_NOT_FOUND;
}

bool camera_manager_has_photo(void) {
    bool has_photo = false;
    
    if (xSemaphoreTake(photo_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        has_photo = (current_photo != NULL);
        xSemaphoreGive(photo_mutex);
    }
    
    return has_photo;
}

camera_info_t camera_manager_get_info(void) {
    return camera_info;
}

uint32_t camera_manager_get_photo_count(void) {
    return camera_info.photo_count;
}

esp_err_t camera_manager_set_quality(int quality) {
    if (quality < 10 || quality > 63) {
        ESP_LOGE(TAG, "Calidad inválida: %d (rango: 10-63)", quality);
        return ESP_ERR_INVALID_ARG;
    }
    
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_quality(s, quality);
        camera_info.jpeg_quality = quality;
        ESP_LOGI(TAG, "Calidad JPEG cambiada a: %d", quality);
        return ESP_OK;
    }
    
    return ESP_FAIL;
}

esp_err_t camera_manager_set_frame_size(framesize_t frame_size) {
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, frame_size);
        camera_info.frame_size = frame_size;
        ESP_LOGI(TAG, "Tamaño de frame cambiado a: %d", frame_size);
        return ESP_OK;
    }
    
    return ESP_FAIL;
}

esp_err_t camera_manager_set_brightness(int brightness) {
    if (brightness < -2 || brightness > 2) {
        ESP_LOGE(TAG, "Brillo inválido: %d (rango: -2 a +2)", brightness);
        return ESP_ERR_INVALID_ARG;
    }
    
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, brightness);
        ESP_LOGI(TAG, "💡 Brillo cambiado a: %d", brightness);
        return ESP_OK;
    }
    
    return ESP_FAIL;
}

esp_err_t camera_manager_optimize_for_low_light(void) {
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        ESP_LOGE(TAG, "No se pudo obtener el sensor");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "🌙 Optimizando cámara para condiciones de poca luz...");
    
    // Configuración moderada para poca luz
    s->set_brightness(s, 2);        // Brillo máximo
    s->set_contrast(s, 1);          // Contraste moderado
    s->set_saturation(s, 0);        // Saturación normal
    s->set_ae_level(s, 2);          // Exposición máxima
    s->set_aec_value(s, 800);       // Tiempo de exposición alto pero no máximo
    s->set_agc_gain(s, 20);         // Ganancia ISO alta pero no máxima
    s->set_gainceiling(s, (gainceiling_t)5); // Techo ganancia alto (64x)
    s->set_aec2(s, 1);              // AEC2 habilitado
    s->set_raw_gma(s, 1);           // Gamma RAW habilitado
    s->set_lenc(s, 1);              // Corrección de lente habilitada
    s->set_bpc(s, 0);               // BPC deshabilitado
    s->set_wpc(s, 1);               // WPC habilitado
    s->set_awb_gain(s, 1);          // AWB gain automático
    s->set_wb_mode(s, 0);           // Modo WB automático
    s->set_dcw(s, 1);               // DCW habilitado
    
    ESP_LOGI(TAG, "✅ Optimización NOCTURNA completada");
    return ESP_OK;
}

esp_err_t camera_manager_optimize_for_daylight(void) {
    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        ESP_LOGE(TAG, "No se pudo obtener el sensor");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "☀️ Optimizando cámara para condiciones DIURNAS...");
    
    // Configuración balanceada para luz del día
    s->set_brightness(s, 0);        // Brillo normal
    s->set_contrast(s, 1);          // Contraste moderado
    s->set_saturation(s, 0);        // Saturación normal
    s->set_ae_level(s, 0);          // Exposición normal
    s->set_aec_value(s, 300);       // Tiempo de exposición normal
    s->set_agc_gain(s, 5);          // Ganancia ISO baja
    s->set_gainceiling(s, (gainceiling_t)2); // Techo ganancia normal (8x)
    s->set_aec2(s, 0);              // AEC2 deshabilitado
    s->set_raw_gma(s, 1);           // Gamma RAW habilitado
    s->set_lenc(s, 1);              // Corrección de lente habilitada
    s->set_bpc(s, 0);               // BPC deshabilitado
    s->set_wpc(s, 1);               // WPC habilitado
    s->set_awb_gain(s, 1);          // AWB gain automático
    s->set_wb_mode(s, 0);           // Modo WB automático
    s->set_dcw(s, 1);               // DCW habilitado

    ESP_LOGI(TAG, "✅ Optimización DIURNA completada");
    return ESP_OK;
}

esp_err_t camera_manager_auto_optimize_lighting(void) {
    ESP_LOGI(TAG, "🔍 Detectando condiciones de luz automáticamente...");
    
    // Tomar una foto de prueba para analizar el brillo
    camera_fb_t *test_photo = esp_camera_fb_get();
    if (!test_photo) {
        ESP_LOGW(TAG, "No se pudo tomar foto de prueba, usando configuración nocturna");
        return camera_manager_optimize_for_low_light();
    }
    
    // Analizar el brillo promedio de la imagen
    uint32_t brightness_sum = 0;
    uint32_t pixel_count = test_photo->len / 3; // Aproximación para análisis
    
    // Muestrear cada 100 píxeles para velocidad
    for (uint32_t i = 0; i < test_photo->len && i < pixel_count * 3; i += 300) {
        brightness_sum += test_photo->buf[i];
    }
    
    uint8_t avg_brightness = brightness_sum / (pixel_count / 100);
    esp_camera_fb_return(test_photo);
    
    ESP_LOGI(TAG, "💡 Brillo promedio detectado: %d/255", avg_brightness);
    
    // Decidir configuración basada en el brillo
    if (avg_brightness < 80) {  // Umbral moderado para poca luz
        ESP_LOGI(TAG, "🌃 Condiciones de POCA LUZ detectadas");
        return camera_manager_optimize_for_low_light();
    } else {
        ESP_LOGI(TAG, "🌅 Condiciones de BUENA LUZ detectadas");
        return camera_manager_optimize_for_daylight();
    }
}

esp_err_t camera_manager_set_night_mode(bool night_mode) {
    if (night_mode) {
        ESP_LOGI(TAG, "🌙 Activando modo NOCTURNO manualmente...");
        return camera_manager_optimize_for_low_light();
    } else {
        ESP_LOGI(TAG, "☀️ Activando modo DIURNO manualmente...");
        return camera_manager_optimize_for_daylight();
    }
}

esp_err_t camera_manager_set_server_queue(QueueHandle_t queue) {
    server_queue = queue;
    ESP_LOGI(TAG, "Cola del servidor web configurada");
    return ESP_OK;
}

esp_err_t camera_manager_deinit(void) {
    ESP_LOGI(TAG, "Desinicializando cámara...");
    
    // Liberar foto actual
    if (xSemaphoreTake(photo_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (current_photo) {
            esp_camera_fb_return(current_photo);
            current_photo = NULL;
        }
        xSemaphoreGive(photo_mutex);
    }
    
    // Desinicializar cámara
    esp_err_t err = esp_camera_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error desinicializando cámara: %s", esp_err_to_name(err));
    }
    
    // Limpiar referencias
    server_queue = NULL;
    
    // Liberar mutex
    if (photo_mutex) {
        vSemaphoreDelete(photo_mutex);
        photo_mutex = NULL;
    }
    
    // Reset información
    memset(&camera_info, 0, sizeof(camera_info));
    
    ESP_LOGI(TAG, "Cámara desinicializada");
    return err;
}