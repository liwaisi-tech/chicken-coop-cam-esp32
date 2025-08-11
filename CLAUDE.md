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
- Automatic photo upload to Google Drive
- WhatsApp notifications via CallMeBot
- 10-second cooldown between photos
- HD JPEG image quality maintained
- GMT-5 timezone support

### 🔄 FLUJO DE EJECUCIÓN DETALLADO

1. **Detección** → Sensor E18 detecta movimiento
2. **Confirmación** → Espera 1s y re-verifica presencia  
3. **Cooldown Check** → Si no han pasado 10s desde última foto, ignora
4. **Estado Check** → Si ya está procesando otra foto, ignora
5. **Alerta Inmediata** → CallMeBot: "🚨 Movimiento detectado en gallinero 15/01/2024 2:30 PM - Imagen cargando..."
6. **Captura** → ESP32-CAM toma foto HD JPEG
7. **Upload** → Google Drive con 3 reintentos
8. **Confirmación** → CallMeBot: "📸 Imagen lista: https://drive.google.com/file/d/XXXXXX"
9. **Error Recovery** → Si falla upload después de 3 intentos: CallMeBot: "❌ Error cargando imagen 15/01/2024 2:30 PM"

### ESPECIFICACIÓN DETALLADA DE NUEVOS COMPONENTES

#### 📦 Componente: `ntp_time`
```c
// include/ntp_time.h
esp_err_t ntp_time_init(void);
char* ntp_get_formatted_time(void);     // "15/01/2024 2:30 PM"
char* ntp_get_filename_time(void);      // "15-01-2024_14-30-25"
char* ntp_get_folder_date(void);        // "2024-ENE"
```

#### 📦 Componente: `google_drive_client`
```c
// include/google_drive_client.h
typedef struct {
    bool upload_success;
    char file_url[512];
    char error_message[256];
} gdrive_upload_result_t;

esp_err_t gdrive_init(void);
gdrive_upload_result_t gdrive_upload_photo(uint8_t* jpeg_data, size_t jpeg_size, const char* filename);
esp_err_t gdrive_cleanup_old_files(void);  // Limpieza mensual
```

#### 📦 Componente: `callmebot_client`  
```c
// include/callmebot_client.h
esp_err_t callmebot_init(void);
esp_err_t callmebot_send_alert(const char* timestamp);           // Mensaje inmediato
esp_err_t callmebot_send_photo_url(const char* url);            // Segundo mensaje
esp_err_t callmebot_send_upload_failed(const char* timestamp);   // Error upload
```

#### 📦 Componente: `system_coordinator`
```c
// include/system_coordinator.h
typedef enum {
    SYSTEM_IDLE,
    SYSTEM_DETECTING,
    SYSTEM_CAPTURING, 
    SYSTEM_UPLOADING
} system_state_t;

esp_err_t system_coordinator_init(void);
void system_coordinator_handle_detection(void);   // Callback desde sensor
system_state_t system_coordinator_get_state(void);
```

### MODIFICACIONES A COMPONENTES EXISTENTES

#### 🔧 `sensorE18` - Modificaciones:
- **Nuevo callback**: Integración con `system_coordinator`
- **Lógica de confirmación**: Detectar → delay 1s → re-verificar antes de activar
- **Anti-spam**: Respetar cooldown de 10 segundos

#### 🔧 `web_server` - Nuevos endpoints:
```c
// Nuevos campos en el estado del servidor
typedef struct {
    uint32_t detections_today;
    char last_photo_sent[64];      // "15/01/2024 2:30 PM" 
    bool sd_card_ok;
    bool sd_card_full;
    system_state_t current_state;
} server_status_t;
```

#### 🔧 `main/chicken-coop-cam.c` - Nueva integración:
```c
// Nueva secuencia de inicialización
void app_main(void) {
    // ... inicialización actual ...
    
    // NUEVOS COMPONENTES
    ntp_time_init();
    gdrive_init(); 
    callmebot_init();
    system_coordinator_init();    // ← Orquestador principal
    
    // ... resto igual ...
}
```

### ⚡ RECOMENDACIONES DE IMPLEMENTACIÓN

**Prioridad 1 (Crítico):**
- `ntp_time`: Fundamental para timestamps
- `system_coordinator`: Orquestador principal del flujo
- Modificar `sensorE18` con lógica de confirmación

**Prioridad 2 (Funcional):**
- `callmebot_client`: Notificaciones WhatsApp
- `google_drive_client`: Upload de imágenes
- Modificar `web_server` con nuevos endpoints

**Consideraciones Técnicas:**
- **Memoria**: Usar heap dinámico para buffers de imagen temporales
- **Concurrencia**: FreeRTOS queues para comunicación entre componentes
- **Robustez**: Watchdog timers para recovery automático
- **Testing**: Modo simulación en `sensorE18` para pruebas sin hardware

**Estimación de Desarrollo:** 
- Setup configuración externa: 2-3 horas
- Desarrollo componentes nuevos: 8-10 horas  
- Integración y testing: 4-6 horas
- **Total**: ~15 horas de desarrollo

### Configuration Requirements

**Google Cloud Setup:**
```bash
# Pasos necesarios ANTES de programar:
1. Ir a console.cloud.google.com
2. Crear nuevo proyecto: "ESP32-Gallinero"  
3. Habilitar Google Drive API
4. Crear Service Account: "esp32-gallinero-sa"
5. Generar clave JSON y guardar como "credentials.json"
6. Compartir carpeta Drive con email del Service Account
```

**CallMeBot WhatsApp Setup:**
```bash
# Configuración CallMeBot:
1. Agregar contacto: +34 644 59 71 67
2. Enviar WhatsApp: "I allow callmebot to send me messages"
3. Guardar API Key recibida
4. Formato URL: https://api.callmebot.com/whatsapp.php?phone=+57XXXXXXXXX&text=mensaje&apikey=XXXXXX
```

**ESP-IDF Configuration:**
```bash
# Agregar a sdkconfig o menuconfig:
CONFIG_GOOGLE_DRIVE_SERVICE_ACCOUNT_EMAIL="esp32-gallinero-sa@esp32-gallinero.iam.gserviceaccount.com"
CONFIG_CALLMEBOT_PHONE_NUMBER="+57XXXXXXXXX" 
CONFIG_CALLMEBOT_API_KEY="XXXXXX"
CONFIG_NTP_SERVER="pool.ntp.org"
CONFIG_TIMEZONE="GMT-5"
```

### Directory Structure
Google Drive folder organization:
- `monitoreoESP32CAM/Gallinero/2024-ENE/gallinero_15-01-2024_14-30-25.jpg`
- Automatic monthly cleanup
- No local photo storage (RAM only during processing)