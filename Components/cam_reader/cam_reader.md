# 📷 cam_reader — Gestor Avanzado de Cámara para ESP32-CAM

## Propósito

El componente `cam_reader` es un gestor de cámara inteligente que proporciona una interfaz completa y *thread-safe* para el sistema de monitoreo de gallineros:

- **Inicialización Inteligente**: Configura automáticamente la cámara ESP32-CAM con parámetros optimizados
- **Captura Automática**: Toma fotos por eventos del sensor infrarrojo E18-D80NK
- **Optimización de Iluminación**: Detecta condiciones de luz y ajusta automáticamente la cámara
- **Gestión de Memoria**: Almacena fotos en RAM con limpieza automática
- **Integración con Web Server**: Notifica al servidor web sobre nuevas fotos capturadas
- **Configuración Dinámica**: Permite ajustar calidad, tamaño y otros parámetros en tiempo real

---

## 🔧 Características Principales

### **Thread-Safety y Concurrencia**
- Uso de `mutex` (semáforos) para proteger acceso concurrente
- Frame buffers protegidos contra race conditions
- Operaciones atómicas para estadísticas de fotos

### **Gestión Inteligente de Memoria**  
- Liberación automática de fotos anteriores
- Buffer único para la foto más reciente
- Gestión eficiente de memoria RAM limitada

### **Optimización de Iluminación Automática**
- **Detección automática** de condiciones de luz
- **Modo diurno**: Configuración optimizada para buena iluminación
- **Modo nocturno**: Configuración para condiciones de poca luz
- **Auto-optimización**: Análisis del histograma de la imagen para ajustes automáticos

### **Integración con Sistema de Eventos**
- Notificación automática al web server cuando se toma una foto
- Cola de eventos para comunicación asíncrona entre componentes
- Timestamping de fotos para seguimiento temporal

### **Configuración Flexible**
- Calidad JPEG ajustable (10-63)
- Múltiples tamaños de frame (desde QQVGA hasta HD)
- Configuración de brillo (-2 a +2)
- Parámetros personalizables vía estructura de configuración

---

## 🚀 API Principal

### **Inicialización**
```c
esp_err_t camera_manager_init(void);
esp_err_t camera_manager_init_with_config(const camera_config_custom_t *config);
```

### **Captura de Fotos**
```c
esp_err_t camera_manager_take_photo(const char* reason);
camera_fb_t* camera_manager_get_current_photo(void);
esp_err_t camera_manager_get_photo_data(uint8_t** buffer, size_t* len);
bool camera_manager_has_photo(void);
```

### **Información y Estadísticas**
```c
camera_info_t camera_manager_get_info(void);
uint32_t camera_manager_get_photo_count(void);
```

### **Configuración Dinámica**
```c
esp_err_t camera_manager_set_quality(int quality);
esp_err_t camera_manager_set_frame_size(framesize_t frame_size);
esp_err_t camera_manager_set_brightness(int brightness);
```

### **Optimización de Iluminación**
```c
esp_err_t camera_manager_auto_optimize_lighting(void);
esp_err_t camera_manager_optimize_for_low_light(void);
esp_err_t camera_manager_optimize_for_daylight(void);
esp_err_t camera_manager_set_night_mode(bool night_mode);
```

---

## 🔌 Integración con el Sistema

### **Sensor E18-D80NK → Cámara**
```c
// Cuando el sensor detecta un objeto
sensor_e18_start_detection_task() → camera_manager_take_photo("detección automática")
```

### **Cámara → Web Server**
```c
// Notificación automática al servidor web
camera_manager_set_server_queue(server_queue);
// Cuando se toma foto → Evento enviado al servidor web
```

### **Main Application → Cámara**
```c
// Inicialización del sistema
camera_manager_init();
camera_manager_auto_optimize_lighting(); // Detección automática de luz
```

### **Web Server → Cámara**
```c
// Servir la foto más reciente
camera_manager_get_photo_data(&buffer, &len);
// Estado para endpoint /status
camera_info_t info = camera_manager_get_info();
```

---

## ⚙️ Configuración de Hardware

### **Pines ESP32-CAM (Definidos en cam_reader.c)**
```c
#define CAM_PIN_PWDN    32  // Power down
#define CAM_PIN_RESET   -1  // Reset (no usado)
#define CAM_PIN_XCLK    0   // Clock externo
#define CAM_PIN_SIOD    26  // I2C Data
#define CAM_PIN_SIOC    27  // I2C Clock
// ... más pines de datos de la cámara
#define CONFIG_XCLK_FREQ 10000000  // 10MHz para estabilidad
```

### **Configuración por Defecto**
- **Frame Size**: HD (1280x720)
- **Calidad JPEG**: 12 (buena calidad)
- **Formato**: JPEG
- **Frame Buffers**: 2 (para mejor rendimiento)

---

## 🧠 Lógica de Optimización Automática

### **Detección de Condiciones de Luz**
1. **Captura foto de muestra** para análisis
2. **Análisis del histograma** de la imagen
3. **Determinación automática** de modo (día/noche)
4. **Aplicación de configuración optimizada**

### **Parámetros de Optimización**
- **Modo Diurno**: Brillo normal, exposición estándar
- **Modo Nocturno**: Mayor brillo, exposición extendida, reducción de ruido

---

## 📊 Estadísticas y Monitoreo

El componente mantiene estadísticas detalladas:
- **photos_taken**: Contador total de fotos
- **last_photo_size**: Tamaño de la última foto
- **last_photo_time**: Timestamp de la última captura
- **initialized**: Estado de inicialización
- **frame_size, jpeg_quality**: Configuración actual

---

## 🔧 Ejemplo de Uso Completo

```c
// 1. Inicialización
camera_manager_init();
camera_manager_set_server_queue(web_server_queue);
camera_manager_auto_optimize_lighting();

// 2. El sensor detecta → foto automática
// (manejado internamente por el sistema de eventos)

// 3. Web server solicita foto
uint8_t* photo_buffer;
size_t photo_len;
if (camera_manager_get_photo_data(&photo_buffer, &photo_len) == ESP_OK) {
    // Enviar foto al cliente web
    httpd_resp_send(req, (char*)photo_buffer, photo_len);
}

// 4. Verificar estadísticas
camera_info_t info = camera_manager_get_info();
ESP_LOGI(TAG, "Fotos tomadas: %lu", info.photos_taken);
```

---

## 🎯 Optimizado para Gallineros

Este componente está específicamente diseñado para el monitoreo de gallineros:
- **Detección automática** cuando las gallinas se acercan
- **Optimización de luz** para interior/exterior del gallinero
- **Memoria eficiente** para dispositivos embebidos
- **Integración web** para monitoreo remoto
- **Operación 24/7** con manejo robusto de errores
