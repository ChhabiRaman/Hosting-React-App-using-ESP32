idf_component_register(SRCS "network.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES nvs_flash esp_wifi esp_eth)
