idf_component_register(SRCS "cam_reader.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "nvs_flash" "esp32-camera")
