#include "unity.h"
#include "cam_reader.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TEST_CAM_READER";

void setUp(void) {
    // This is run before EACH test
}

void tearDown(void) {
    // This is run after EACH test
    camera_manager_deinit();
}

void test_cam_reader_init(void) {
    ESP_LOGI(TAG, "Testing camera reader initialization");
    
    // Test successful initialization
    esp_err_t result = camera_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test that camera info is available
    camera_info_t info = camera_manager_get_info();
    TEST_ASSERT_TRUE(info.initialized);
}

void test_cam_reader_config(void) {
    ESP_LOGI(TAG, "Testing camera reader configuration");
    
    // Initialize camera first
    TEST_ASSERT_EQUAL(ESP_OK, camera_manager_init());
    
    // Test initial photo count
    uint32_t initial_count = camera_manager_get_photo_count();
    TEST_ASSERT_EQUAL(0, initial_count);
    
    // Test camera info
    camera_info_t info = camera_manager_get_info();
    TEST_ASSERT_TRUE(info.initialized);
    TEST_ASSERT_EQUAL(0, info.photos_taken);
    
    ESP_LOGI(TAG, "Camera initialized successfully, ready for photo capture");
}