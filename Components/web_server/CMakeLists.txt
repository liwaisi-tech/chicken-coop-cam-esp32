idf_component_register(SRCS "web_server.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_http_server"
                    PRIV_REQUIRES "driver" "freertos" "cam_reader")
