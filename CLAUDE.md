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