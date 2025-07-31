#ifndef CAM_READER_H
#define CAM_READER_H

#include "esp_err.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Información de la cámara
typedef struct {
    bool initialized;
    framesize_t frame_size;
    pixformat_t pixel_format;
    int jpeg_quality;
    uint32_t photo_count;
    size_t last_photo_size;
    uint64_t last_photo_time;
} camera_info_t;

// Configuración de la cámara
typedef struct {
    framesize_t frame_size;
    int jpeg_quality;
    pixformat_t pixel_format;
    int fb_count;
} camera_config_custom_t;

#define CAMERA_DEFAULT_CONFIG() { \
    .frame_size = FRAMESIZE_HD, \
    .jpeg_quality = 12, \
    .pixel_format = PIXFORMAT_JPEG, \
    .fb_count = 2 \
}

/**
 * @brief Inicializa el componente de cámara con configuración por defecto
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_init(void);

/**
 * @brief Inicializa el componente de cámara con configuración personalizada
 * @param config Configuración personalizada de la cámara
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_init_with_config(const camera_config_custom_t *config);

/**
 * @brief Toma una foto y la almacena internamente
 * @param reason Razón por la cual se toma la foto (para logging)
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_take_photo(const char* reason);

/**
 * @brief Obtiene puntero al frame buffer de la foto actual (uso directo)
 * @warning El puntero puede ser NULL si no hay foto
 * @warning No liberar el frame buffer retornado
 * @return Puntero al frame buffer actual o NULL
 */
camera_fb_t* camera_manager_get_current_photo(void);

/**
 * @brief Obtiene los datos de la foto actual de forma segura
 * @param buffer Puntero donde almacenar la dirección del buffer
 * @param len Puntero donde almacenar el tamaño del buffer
 * @return ESP_OK si hay foto disponible, ESP_ERR_NOT_FOUND si no hay foto
 */
esp_err_t camera_manager_get_photo_data(uint8_t** buffer, size_t* len);

/**
 * @brief Verifica si hay una foto disponible
 * @return true si hay foto, false en caso contrario
 */
bool camera_manager_has_photo(void);

/**
 * @brief Obtiene información general de la cámara
 * @return Estructura con información de la cámara
 */
camera_info_t camera_manager_get_info(void);

/**
 * @brief Obtiene el número total de fotos tomadas
 * @return Número de fotos tomadas desde la inicialización
 */
uint32_t camera_manager_get_photo_count(void);

/**
 * @brief Cambia la calidad JPEG de las fotos
 * @param quality Calidad JPEG (10-63, menor número = mejor calidad)
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_set_quality(int quality);

/**
 * @brief Cambia el tamaño del frame de las fotos
 * @param frame_size Nuevo tamaño del frame
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_set_frame_size(framesize_t frame_size);

/**
 * @brief Ajusta el brillo de la cámara para mejorar la iluminación
 * @param brightness Brillo (-2 a +2, donde +2 es más brillante)
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_set_brightness(int brightness);

/**
 * @brief Optimiza automáticamente la cámara para condiciones de poca luz
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_optimize_for_low_light(void);

/**
 * @brief Optimiza automáticamente la cámara para condiciones de buena luz
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_optimize_for_daylight(void);

/**
 * @brief Detecta automáticamente las condiciones de luz y optimiza la cámara
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_auto_optimize_lighting(void);

/**
 * @brief Cambia entre modo diurno y nocturno manualmente para pruebas
 * @param night_mode true para modo nocturno, false para modo diurno
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_set_night_mode(bool night_mode);

/**
 * @brief Configura la cola del servidor web para notificaciones
 * @param queue Handle de la cola del servidor web
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_set_server_queue(QueueHandle_t queue);

/**
 * @brief Desinicializa el componente de cámara y libera recursos
 * @return ESP_OK si exitoso, código de error en caso contrario
 */
esp_err_t camera_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // CAM_READER_H