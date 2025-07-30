# 📷 cam_reader — Gestor de Cámara para ESP32-CAM

## Propósito

El componente `cam_reader` es un gestor de cámara que proporciona una interfaz limpia y *thread-safe* para:

- Inicializar la cámara **ESP32-CAM**  
- Capturar fotos y almacenarlas en **RAM**  
- Gestionar el acceso concurrente a los datos  
- Configurar parámetros de **calidad y tamaño**  
- Proporcionar información de **estado**

---

## 🔧 Características Principales

- **Thread-Safe**: Usa `mutex` para proteger el acceso concurrente  
- **Gestión de Memoria**: Libera automáticamente fotos anteriores  
- **Configuración Dinámica**: Permite cambiar calidad y tamaño en tiempo real  
- **Logging Detallado**: Registra todas las operaciones importantes  
- **Manejo de Errores**: Validaciones y *timeouts* robustos

---

## Flujo de Trabajo

1. **Inicialización**  
   `camera_manager_init()` → Configura pines y parámetros

2. **Captura**  
   `camera_manager_take_photo()` → Toma foto y la almacena

3. **Acceso**  
   `camera_manager_get_photo_data()` → Obtiene datos de forma segura

4. **Configuración**  
   `camera_manager_set_quality()` → Ajusta parámetros

5. **Limpieza**  
   `camera_manager_deinit()` → Libera recursos

---

## 🔌 Integración con el Sistema

- **Sensor**: El sensor `E18-D80NK` puede llamar a `camera_manager_take_photo()`  
- **WiFi**: Otros componentes pueden acceder a los datos de la foto para enviarlos  
- **Main**: El archivo principal coordina sensor + cámara + comunicación
