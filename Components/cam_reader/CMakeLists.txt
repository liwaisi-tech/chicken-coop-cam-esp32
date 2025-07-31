idf_component_register(SRCS "cam_reader.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "espressif__esp32-camera" "esp_timer" "web_server"
                    PRIV_REQUIRES "nvs_flash")
