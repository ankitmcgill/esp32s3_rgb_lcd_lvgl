idf_component_register(SRCS "main.c"
                        REQUIRES
                            driver_chipinfo
                            driver_appinfo
                            driver_spiffs
                            util_dataqueue
                            defines
                            project_defines
                        PRIV_REQUIRES
                            spi_flash
                            nvs_flash
                            log
                            freertos
                        INCLUDE_DIRS "")

# Generate Spiffs Partition Binary From "spiffs_fs" folder And Flash To Controller
spiffs_create_partition_image(spiff 
    "../../../spiffs_fs" 
    FLASH_IN_PROJECT
)
