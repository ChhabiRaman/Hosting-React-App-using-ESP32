idf_component_register(SRCS "webpage.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server vfs
                    PRIV_REQUIRES esp-tls json vfs_storage)
