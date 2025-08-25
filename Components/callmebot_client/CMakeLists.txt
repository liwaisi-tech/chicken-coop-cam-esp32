idf_component_register(SRCS "callmebot_client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_client
                    PRIV_REQUIRES esp_netif esp_wifi esp-tls)