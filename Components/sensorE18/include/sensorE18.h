#ifndef SENSOR_E18_H
#define SENSOR_E18_H

#include "driver/gpio.h"
#include "esp_log.h"

// Estructura con valores por defecto para la configuración del sensor
typedef struct {
    gpio_num_t pin;                    // Pin GPIO (por defecto GPIO_NUM_15)
    gpio_pullup_t pull_up_en;         // Pull-up habilitado por defecto
    gpio_pulldown_t pull_down_en;     // Pull-down deshabilitado por defecto
    gpio_int_type_t intr_type;        // Interrupción en flanco descendente por defecto
} sensor_e18_config_t;

// Valores por defecto para la configuración
#define SENSOR_E18_DEFAULT_CONFIG { \
    .pin = GPIO_NUM_13, \
    .pull_up_en = GPIO_PULLUP_ENABLE, \
    .pull_down_en = GPIO_PULLDOWN_DISABLE, \
    .intr_type = GPIO_INTR_ANYEDGE \
}

// Declaración de la función de inicialización
void init_sensor(void);

// Función para inicializar con configuración personalizada
void init_sensor_with_config(const sensor_e18_config_t *config);

#endif // SENSOR_E18_H
