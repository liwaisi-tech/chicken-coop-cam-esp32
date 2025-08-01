# 🐔 Sistema de Monitoreo ESP32-CAM para Gallinero

Este proyecto implementa un sistema inteligente de monitoreo para gallineros utilizando **ESP32-CAM** y sensor infrarrojo **E18-D80NK**. El sistema detecta automáticamente la presencia de gallinas y captura fotos que pueden ser visualizadas a través de una interfaz web.

## 📋 Características Principales

- **Detección Automática**: Sensor infrarrojo E18-D80NK detecta la presencia de objetos/animales
- **Captura de Fotos**: ESP32-CAM toma fotos automáticamente cuando se detecta movimiento
- **Interfaz Web**: Servidor HTTP para visualizar fotos y estado del sistema
- **Conectividad WiFi**: Acceso remoto desde cualquier dispositivo conectado
- **Monitoreo en Tiempo Real**: Estadísticas y estado del sistema disponibles 24/7
- **Arquitectura Modular**: Componentes independientes para fácil mantenimiento

## 🛠️ Hardware Requerido

- **ESP32-CAM** - Módulo con cámara integrada
- **Sensor E18-D80NK** - Sensor infrarrojo de proximidad
- **Fuente de alimentación** 5V para ESP32-CAM
- **Cables de conexión**

## 🔧 Configuración del Hardware

### Conexiones del Sensor E18-D80NK:
- **VCC**: 5V
- **GND**: Ground
- **OUT**: GPIO configurado en `sensorE18.h`

### ESP32-CAM:
- Utiliza configuración de pines estándar para la cámara
- LED integrado para indicación de estado

## 🚀 Instalación y Compilación

### Requisitos Previos:
- ESP-IDF v5.4.2 o superior
- Python 3.7+
- Git

### Pasos de Instalación:

1. **Clonar el repositorio:**
```bash
git clone <repository-url>
cd chicken-coop-cam-esp32
```

2. **Configurar el sistema:**
```bash
idf.py menuconfig
```

### Configuraciones Críticas para ESP32-CAM:

**A. Component config → Camera configuration:**
- ✅ `OV2640 Support` = Enable
- ✅ `OV3660 Support` = Enable  
- ✅ `OV5640 Support` = Enable
- ✅ `OV7670 Support` = Enable
- ✅ `OV7725 Support` = Enable
- `Camera task stack size` = 2048
- `Camera task priority` = 0 (Core 0)
- `Camera DMA buffer size` = 32768

**B. Component config → ESP32-specific → SPI RAM config:**
- ✅ `Support for external, SPI-connected RAM` = Enable
- `Type of SPI RAM chip in use` = Auto-detect
- `Set RAM clock speed` = 40MHz
- `SPI RAM access method` = Use CAPS_ALLOC
- ✅ `Initialize SPI RAM during startup` = Enable
- ✅ `Enable workaround for bug in SPI RAM cache` = Enable

**C. Serial flasher config:**
- `Flash size` = 2MB (actual configuration)

**D. Partition Table:**
- `Partition Table` = Single factory app, no OTA

**E. Component config → Wi-Fi:**
- `WiFi static RX buffer number` = 10
- `WiFi dynamic RX buffer number` = 32
- `WiFi dynamic TX buffer number` = 32

**F. WiFi Configuration (crear en menuconfig):**
- SSID de tu red WiFi
- Contraseña de la red WiFi

**G. Component config → FreeRTOS:**
- `Tick rate (Hz)` = 100

3. **Compilar el proyecto:**
```bash
idf.py build
```

4. **Flashear al ESP32-CAM:**
```bash
idf.py flash
```

5. **Monitorear la salida:**
```bash
idf.py monitor
```

## 🌐 Uso del Sistema

### Acceso Web:
1. Una vez iniciado, el sistema mostrará la IP asignada en el monitor serial
2. Acceder desde un navegador: `http://[IP_DEL_ESP32]`
3. Endpoints disponibles:
   - `/` - Página principal
   - `/photo` - Última foto capturada
   - `/status` - Estado del sistema en formato JSON

### Operación Automática:
- El sistema funciona continuamente detectando objetos
- Las fotos se toman automáticamente cuando se detecta presencia
- El monitoreo se registra cada 30 segundos en el log serial

## 🧪 Testing

El proyecto incluye tests unitarios para validar componentes:

```bash
# Ejecutar tests
cd test && idf.py build flash monitor
```

Los tests validan:
- Inicialización del sensor E18-D80NK
- Configuración de GPIO
- Inicialización de la cámara
- Operaciones básicas de los componentes

## 📊 Monitoreo y Logs

El sistema proporciona información detallada a través del monitor serial:
- Estado de inicialización de componentes
- Detecciones en tiempo real
- Estadísticas de fotos capturadas
- Estado de conexión WiFi
- Diagnósticos del sensor

## 🔍 Solución de Problemas

### Problemas Comunes:

**Cámara no inicializa:**
- Verificar conexiones de alimentación (5V estable)
- Revisar que el módulo ESP32-CAM esté correctamente conectado

**Sensor no detecta:**
- Verificar conexión del pin OUT del E18-D80NK
- Comprobar alimentación del sensor (5V)
- Revisar configuración de GPIO en código

**No conecta a WiFi:**
- Verificar credenciales en `menuconfig`
- Comprobar cobertura de la red WiFi
- Revisar configuración de router (WPA2)

## 📁 Estructura del Proyecto

```
chicken-coop-cam-esp32/
├── main/                    # Aplicación principal
│   ├── chicken-coop-cam.c   # Coordinador del sistema
│   └── idf_component.yml    # Dependencias
├── components/              # Componentes modulares
│   ├── cam_reader/          # Gestor de cámara
│   ├── sensorE18/           # Driver del sensor infrarrojo
│   ├── web_server/          # Servidor HTTP
│   └── wifi/                # Conectividad WiFi
├── test/                    # Tests unitarios
├── managed_components/      # Componentes de ESP-IDF
└── CLAUDE.md               # Documentación para desarrollo
```

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor:
1. Fork del proyecto
2. Crear rama para nueva funcionalidad
3. Commit de cambios
4. Push a la rama
5. Crear Pull Request

## 📄 Licencia

Este proyecto está bajo la licencia especificada en el archivo LICENSE.

## 🔗 Recursos Adicionales

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP32-CAM Documentation](https://github.com/espressif/esp32-camera)
- [E18-D80NK Sensor Datasheet](https://www.google.com/search?q=E18-D80NK+datasheet)

---

**Desarrollado para monitoreo inteligente de gallineros 🐔📸**
