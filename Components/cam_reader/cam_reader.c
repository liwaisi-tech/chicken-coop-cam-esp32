// cam_reader.c - Responsabilidad única: gestión de la cámara
#include "cam_reader.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
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
#define CONFIG_XCLK_FREQ 20000000

static const char *TAG = "CAMERA_MANAGER";

// Variables privadas del módulo
static camera_fb_t *current_photo = NULL;
static SemaphoreHandle_t photo_mutex = NULL;
static camera_info_t camera_info = {0};

esp_err_t camera_manager_init(void) {
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
        
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_SVGA,  // 800x600
        .jpeg_quality = 45,
        .fb_count = 1,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY
    };
    
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando cámara: %s", esp_err_to_name(err));
        return err;
    }
    
    // Inicializar información de la cámara
    camera_info.initialized = true;
    camera_info.frame_size = FRAMESIZE_SVGA;
    camera_info.pixel_format = PIXFORMAT_JPEG;
    camera_info.jpeg_quality = 45;
    
    ESP_LOGI(TAG, "Cámara inicializada correctamente");
    return ESP_OK;
}

esp_err_t camera_manager_take_photo(const char* reason) {
    if (!camera_info.initialized) {
        ESP_LOGE(TAG, "Cámara no inicializada");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Tomando foto por: %s", reason ? reason : "razón no especificada");
    
    camera_fb_t *new_photo = esp_camera_fb_get();
    if (!new_photo) {
        ESP_LOGE(TAG, "Error capturando foto");
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
        
        ESP_LOGI(TAG, "Nueva foto #%lu almacenada - Tamaño: %zu bytes", 
                camera_info.photo_count, current_photo->len);
        
        xSemaphoreGive(photo_mutex);
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