// sensorE18.h - Header para gestión del sensor E18-D80NK
#ifndef SENSOR_E18_H
#define SENSOR_E18_H

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Estructura de configuración del sensor
typedef struct {
    gpio_num_t pin;                    // Pin GPIO
    gpio_pullup_t pull_up_en;         // Pull-up habilitado
    gpio_pulldown_t pull_down_en;     // Pull-down deshabilitado
    gpio_int_type_t intr_type;        // Tipo de interrupción
} sensor_e18_config_t;

// Estructura de estadísticas del sensor
typedef struct {
    uint32_t detection_count;         // Contador de detecciones
    bool object_detected;             // Estado actual (objeto detectado)
    int64_t last_detection_time;      // Tiempo de última detección (microsegundos)
} sensor_statistics_t;

// Configuración por defecto
#define SENSOR_E18_DEFAULT_CONFIG { \
    .pin = GPIO_NUM_13, \
    .pull_up_en = GPIO_PULLUP_ENABLE, \
    .pull_down_en = GPIO_PULLDOWN_DISABLE, \
    .intr_type = GPIO_INTR_ANYEDGE \
}

/**
 * @brief Inicializar sensor con configuración por defecto
 * @return ESP_OK si exitoso
 */
esp_err_t sensor_e18_init(void);

/**
 * @brief Inicializar sensor con configuración personalizada
 * @param config Configuración del sensor
 * @return ESP_OK si exitoso
 */
esp_err_t sensor_e18_init_with_config(const sensor_e18_config_t *config);

/**
 * @brief Iniciar la tarea de detección
 * @return ESP_OK si exitoso
 */
esp_err_t sensor_e18_start_detection_task(void);

/**
 * @brief Obtener estadísticas actuales del sensor
 * @return Estructura con estadísticas
 */
sensor_statistics_t sensor_e18_get_statistics(void);

/**
 * @brief Leer estado actual del sensor (sin interrupción)
 * @return 1 si objeto detectado, 0 si no
 */
int sensor_e18_read_state(void);

/**
 * @brief Obtener configuración actual del sensor
 * @return Estructura de configuración
 */
sensor_e18_config_t sensor_e18_get_config(void);

/**
 * @brief Desinicializar el sensor y liberar recursos
 * @return ESP_OK si exitoso
 */
esp_err_t sensor_e18_deinit(void);


/**
 * @brief Simular detección de objeto para pruebas
 * @param simulate_detection true para simular detección, false para simular sin objeto
 * @return ESP_OK si exitoso
 */
esp_err_t sensor_e18_simulate_detection(bool simulate_detection);

/**
 * @brief Definir tipo de callback para detección confirmada
 */
typedef void (*motion_detected_callback_t)(void);

/**
 * @brief Configurar callback para cuando se confirma detección
 * @param callback Función a llamar cuando se detecta movimiento confirmado
 * @return ESP_OK si exitoso
 */
esp_err_t sensor_e18_set_callback(motion_detected_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_E18_H