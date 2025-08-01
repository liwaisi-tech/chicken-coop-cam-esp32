#include "unity.h"
#include "sensorE18.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "TEST_SENSOR_E18";

void setUp(void) {
    // This is run before EACH test
}

void tearDown(void) {
    // This is run after EACH test
    sensor_e18_deinit();
}

void test_sensor_e18_init(void) {
    ESP_LOGI(TAG, "Testing sensor E18 initialization");
    
    // Test successful initialization
    esp_err_t result = sensor_e18_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test that configuration is properly set
    sensor_e18_config_t config = sensor_e18_get_config();
    TEST_ASSERT_NOT_EQUAL(0, config.pin);
    TEST_ASSERT_NOT_EQUAL(0, config.gpio_pin);
}

void test_sensor_e18_config(void) {
    ESP_LOGI(TAG, "Testing sensor E18 configuration");
    
    // Initialize sensor first
    TEST_ASSERT_EQUAL(ESP_OK, sensor_e18_init());
    
    // Get default configuration
    sensor_e18_config_t config = sensor_e18_get_config();
    TEST_ASSERT_NOT_EQUAL(0, config.pin);
    TEST_ASSERT_NOT_EQUAL(0, config.gpio_pin);
    
    // Test statistics initialization
    sensor_statistics_t stats = sensor_e18_get_statistics();
    TEST_ASSERT_EQUAL(0, stats.detection_count);
    TEST_ASSERT_FALSE(stats.object_detected);
}

void test_sensor_e18_gpio_operations(void) {
    ESP_LOGI(TAG, "Testing sensor E18 GPIO operations");
    
    // Initialize sensor
    TEST_ASSERT_EQUAL(ESP_OK, sensor_e18_init());
    
    // Test GPIO state reading
    int state = sensor_e18_read_state();
    TEST_ASSERT(state == 0 || state == 1); // Should be either 0 or 1
    
    ESP_LOGI(TAG, "Current GPIO state: %d (0=object detected, 1=no object)", state);
    
    // Test that we can read the state multiple times without error
    for (int i = 0; i < 5; i++) {
        int current_state = sensor_e18_read_state();
        TEST_ASSERT(current_state == 0 || current_state == 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}