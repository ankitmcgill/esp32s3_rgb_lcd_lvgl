idf_component_register(SRCS "module_openweather.cc"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES
                            defines
                            project_defines
                       REQUIRES
                            freertos
)
