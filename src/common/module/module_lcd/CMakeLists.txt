idf_component_register(SRCS "module_lcd.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES
                            driver_lcd
                            defines
                            project_defines
                       REQUIRES
                            freertos
)
