// cam_reader.h - Header para gestión de cámara
#ifndef CAM_READER_H
#define CAM_READER_H

#include "esp_err.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Estructura de información de la cámara
typedef struct {
    bool initialized;                // Estado de inicialización
    uint32_t photo_count;           // Contador de fotos tomadas
    size_t last_photo_size;         // Tamaño de la última foto en bytes
    int64_t last_photo_time;        // Timestamp de la última foto
    framesize_t frame_size;         // Tamaño de frame actual
    pixformat_t pixel_format;       // Formato de pixel
    int jpeg_quality;               // Calidad JPEG actual
} camera_info_t;

/**
 * @brief Inicializar el módulo de cámara
 * @return ESP_OK si exitoso
 */
esp_err_t camera_manager_init(void);

/**
 * @brief Tomar una foto y almacenarla en RAM
 * @param reason Razón para tomar la foto (para logging)
 * @return ESP_OK si exitoso
 */
esp_err_t camera_manager_take_photo(const char* reason);

/**
 * @brief Obtener puntero a la foto actual (NO thread-safe)
 * @warning Solo usar para lectura rápida, no almacenar el puntero
 * @return Puntero a camera_fb_t o NULL si no hay foto
 */
camera_fb_t* camera_manager_get_current_photo(void);

/**
 * @brief Obtener datos de la foto actual de forma thread-safe
 * @param buffer Puntero donde almacenar el buffer de datos
 * @param len Puntero donde almacenar el tamaño
 * @return ESP_OK si exitoso, ESP_ERR_NOT_FOUND si no hay foto
 */
esp_err_t camera_manager_get_photo_data(uint8_t** buffer, size_t* len);

/**
 * @brief Verificar si hay una foto disponible
 * @return true si hay foto, false si no
 */
bool camera_manager_has_photo(void);

/**
 * @brief Obtener información actual de la cámara
 * @return Estructura con información de la cámara
 */
camera_info_t camera_manager_get_info(void);

/**
 * @brief Obtener contador de fotos tomadas
 * @return Número total de fotos tomadas
 */
uint32_t camera_manager_get_photo_count(void);

/**
 * @brief Cambiar la calidad JPEG
 * @param quality Calidad JPEG (10-63, menor número = mejor calidad)
 * @return ESP_OK si exitoso
 */
esp_err_t camera_manager_set_quality(int quality);

/**
 * @brief Cambiar el tamaño de frame
 * @param frame_size Nuevo tamaño de frame
 * @return ESP_OK si exitoso
 */
esp_err_t camera_manager_set_frame_size(framesize_t frame_size);

/**
 * @brief Desinicializar el módulo de cámara
 * @return ESP_OK si exitoso
 */
esp_err_t camera_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // CAM_READER_H