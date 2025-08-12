# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-CAM based chicken coop monitoring system that integrates:
- **ESP32-CAM module** for photo capture
- **E18-D80NK infrared sensor** for object detection
- **Web server** for remote monitoring and control
- **WiFi connectivity** for network access

The system automatically detects objects (chickens) using the infrared sensor and triggers photo capture, serving images through a web interface.

## Build System and Commands

This project uses ESP-IDF (Espressif IoT Development Framework) v5.4.2:

```bash
# Build the project
idf.py build

# Flash to ESP32-CAM
idf.py flash

# Monitor serial output
idf.py monitor

# Clean build
idf.py clean

# Set target (if needed)
idf.py set-target esp32

# Build, flash and monitor in one command
idf.py build flash monitor

# Run unit tests
cd test && idf.py build flash monitor
```

## Component Architecture

The project follows a modular component-based architecture:

### Core Components (`components/`)
- **cam_reader**: Thread-safe camera management with photo capture and memory management
- **sensorE18**: E18-D80NK infrared sensor driver with interrupt-based detection and simulation capabilities
- **web_server**: HTTP server providing REST API and web interface for photo access and system status
- **wifi**: WiFi station connection management

### Main Application (`main/`)
- **chicken-coop-cam.c**: System coordinator that initializes all components and manages the integration loop

### Dependencies
- **espressif/esp32-camera**: Managed component for ESP32-CAM hardware abstraction
- **espressif/esp_jpeg**: JPEG encoding/decoding support

## Key Integration Patterns

### Event-Driven Architecture
Components communicate through FreeRTOS queues:
- Sensor detection events trigger camera captures
- Camera events notify the web server of new photos
- Web server maintains system state and statistics

### Thread Safety
- All components use mutexes for concurrent access protection
- Photo data is managed through semaphore-protected buffers
- Server state updates are atomic

### Configuration Management
- WiFi credentials configured via `idf.py menuconfig` under "WiFi Configuration"
- Camera parameters auto-optimize based on lighting conditions
- Sensor includes both real GPIO detection and simulation modes for testing

## Hardware Configuration

ESP32-CAM pin mapping is defined in `cam_reader.c`:
- Camera pins: Standard ESP32-CAM pinout
- Sensor pin: GPIO configured in `sensorE18.h`
- XCLK frequency: 10MHz for stability

## Development Workflow

1. **Component Modification**: Each component has clear separation of concerns - modify only the relevant component
2. **Testing**: Use `cd test && idf.py build flash monitor` to run unit tests for individual components
3. **Hardware Testing**: The sensor includes a diagnostic test function `sensor_e18_test()` for real GPIO validation
4. **Integration**: Main application coordinates all components - modify `chicken-coop-cam.c` for system-level changes
5. **Web Interface**: Server exposes `/photo`, `/status` endpoints and serves images directly

## Testing Structure

The project includes proper ESP-IDF unit tests in the `test/` directory:
- **test_sensor_e18.c**: Tests real GPIO operations, initialization, and configuration
- **test_cam_reader.c**: Tests camera initialization and configuration
- Tests use Unity framework and can be run independently of the main application

## Memory Management

- Photos are stored in RAM with automatic cleanup of previous captures
- Components use FreeRTOS heap for dynamic allocation
- Managed components handle their own memory lifecycle

## WhatsApp Monitoring System - Architecture Design

### System Requirements
This system extends the current ESP32-CAM functionality to include:
- Motion detection with E18-D80NK sensor (1 second confirmation)
- WhatsApp notifications via CallMeBot with web server URL
- 10-second cooldown between photos
- HD JPEG image quality maintained (via existing web server)
- GMT-5 timezone support
- Direct access to photos through existing web interface

### 🔄 FLUJO DE EJECUCIÓN SIMPLIFICADO

1. **Detección** → Sensor E18 detecta movimiento
2. **Confirmación** → Espera 1s y re-verifica presencia  
3. **Cooldown Check** → Si no han pasado 10s desde última foto, ignora
4. **Captura** → ESP32-CAM toma foto HD JPEG (usa código existente cam_reader)
5. **WhatsApp Alert** → CallMeBot: "🚨 Movimiento detectado en gallinero 📅 15/01/2024 2:30 PM 🌐 Ver imagen: http://192.168.1.100/photo"
6. **Usuario** → Abre URL y ve la foto directamente en el navegador web del ESP32

### ESPECIFICACIÓN DETALLADA DE NUEVOS COMPONENTES (SIMPLIFICADO)

#### 📦 Componente: `ntp_time`
```c
// include/ntp_time.h
esp_err_t ntp_time_init(void);
char* ntp_get_formatted_time(void);     // "15/01/2024 2:30 PM"
```

#### 📦 Componente: `callmebot_client`  
```c
// include/callmebot_client.h
esp_err_t callmebot_init(void);
esp_err_t callmebot_send_detection_alert(const char* timestamp, const char* server_url);

// Ejemplo de mensaje completo:
// "🚨 Movimiento detectado en gallinero
//  📅 15/01/2024 2:30 PM  
//  🌐 Ver imagen: http://192.168.1.100/photo"
```

#### 🔧 Modificación: `main/chicken-coop-cam.c` - Integración simplificada
```c
// Callback desde sensorE18 cuando detecta movimiento:
void on_motion_detected(void) {
    if (cooldown_active()) return;  // 10s cooldown
    
    // Tomar foto (código existente)
    cam_reader_capture_photo();
    
    // Enviar WhatsApp con URL local
    char* timestamp = ntp_get_formatted_time();
    char server_url[64];
    sprintf(server_url, "http://%s/photo", wifi_get_local_ip());
    callmebot_send_detection_alert(timestamp, server_url);
    
    set_cooldown_timer(10000);  // 10 segundos
}
```

### MODIFICACIONES A COMPONENTES EXISTENTES (SIMPLIFICADO)

#### 🔧 `sensorE18` - Modificaciones mínimas:
- **Nuevo callback**: `on_motion_detected()` definido en main
- **Lógica de confirmación**: Detectar → delay 1s → re-verificar antes de activar
- **Integración**: Solo necesita llamar al callback

#### 🔧 `web_server` - Sin cambios necesarios:
- **Endpoint `/photo`** → Ya existe y funciona
- **Endpoint `/status`** → añadir contador de detecciones
- **Componente actual** → Se mantiene igual

#### 🔧 `main/chicken-coop-cam.c` - Inicialización simplificada:
```c
void app_main(void) {
    // ... inicialización actual (cam_reader, sensorE18, web_server, wifi) ...
    
    // NUEVOS COMPONENTES (solo 2)
    ntp_time_init();      // Para timestamps  
    callmebot_init();     // Para WhatsApp
    
    // Configurar callback del sensor
    sensor_e18_set_callback(on_motion_detected);
    
    // ... resto igual ...
}
```

### ⚡ RECOMENDACIONES DE IMPLEMENTACIÓN (SIMPLIFICADO)

**Prioridad 1 (Crítico):**
- `ntp_time`: Timestamps para WhatsApp
- `callmebot_client`: Notificaciones WhatsApp
- Modificar `sensorE18` con callback. cooldown de  10 seg y confirmación con dectección continua de 3 seg

**Prioridad 2 (Opcional):**
- Contador de detecciones en `/status`
- Logs de actividad

**Consideraciones Técnicas Simplificadas:**
- **Reutilización**: Aprovechar código existente de cam_reader y web_server
- **Memoria**: Sin cambios, usa la misma gestión actual
- **Concurrencia**: Callback simple desde sensor, sin queues complejas
- **Testing**: Usar modo simulación existente en sensorE18

**Estimación de Desarrollo Reducida:** 
- Setup CallMeBot: 30 minutos
- Componente `ntp_time`: 1-2 horas
- Componente `callmebot_client`: 2-3 horas  
- Integración y testing: 1-2 horas
- **Total**: ~4-6 horas de desarrollo (vs 15 horas originales)

### Configuration Requirements (SIMPLIFICADO)

**CallMeBot WhatsApp Setup:**
```bash
# Configuración CallMeBot (única configuración externa necesaria):
1. Agregar contacto: +34 644 59 71 67
2. Enviar WhatsApp: "I allow callmebot to send me messages"
3. Guardar API Key recibida
4. Formato URL: https://api.callmebot.com/whatsapp.php?phone=+57XXXXXXXXX&text=mensaje&apikey=XXXXXX
```

**ESP-IDF Configuration:**
```bash
# Configuración super simplificada en sdkconfig:
CONFIG_CALLMEBOT_PHONE_NUMBER="+57XXXXXXXXX"
CONFIG_CALLMEBOT_API_KEY="XXXXXX"
CONFIG_NTP_SERVER="pool.ntp.org"
CONFIG_TIMEZONE="GMT-5"
# Todo lo demás usa la configuración WiFi existente
```

### Sistema de Acceso Directo (SIN UPLOADS EXTERNOS)

**Acceso a fotos:**
- **URL Local**: `http://192.168.1.100/photo` (servidor web existente)
- **Sin almacenamiento externo**: Solo RAM durante captura
- **Acceso inmediato**: Usuario ve foto directamente en navegador
- **Simplicidad máxima**: Reutiliza infraestructura existente

### Ventajas del Diseño Simplificado:
- **Sin configuración compleja**: Solo CallMeBot + NTP
- **Sin APIs externas**: No Cloudinary, no Google Drive, no OAuth2
- **Reutiliza código existente**: cam_reader + web_server ya funcionan
- **Menor latencia**: Acceso directo sin uploads
- **Más confiable**: Menos puntos de falla externos
- **Desarrollo rápido**: ~4-6 horas vs 15 horas originales