#include "unity.h"
#include "esp_system.h"
#include "nvs_flash.h"

// Test function declarations
void test_sensor_e18_init(void);
void test_sensor_e18_config(void);
void test_sensor_e18_gpio_operations(void);
void test_cam_reader_init(void);
void test_cam_reader_config(void);

void app_main(void)
{
    // Initialize NVS for tests that might need it
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("\n=== ESP32-CAM CHICKEN COOP MONITORING - UNIT TESTS ===\n");
    
    UNITY_BEGIN();
    
    // Sensor E18 tests
    RUN_TEST(test_sensor_e18_init);
    RUN_TEST(test_sensor_e18_config);
    RUN_TEST(test_sensor_e18_gpio_operations);
    
    // Camera reader tests
    RUN_TEST(test_cam_reader_init);
    RUN_TEST(test_cam_reader_config);
    
    UNITY_END();
}