idf_component_register(SRCS "driver_openweather.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES
                            esp_http_client
                            json
                            defines
                            util_dataqueue
                            project_defines
                       REQUIRES
                            freertos
)
