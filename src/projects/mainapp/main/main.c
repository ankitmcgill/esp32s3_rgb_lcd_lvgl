#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "module_lcd.h"
#include "module_api.h"
#include "module_wifi.h"
#include "driver_api.h"
#include "driver_wifi.h"
#include "driver_lcd.h"
#include "driver_chipinfo.h"
#include "driver_appinfo.h"
#include "driver_spiffs.h"
#include "define_rtos_tasks.h"
#include "project_defines.h"

static util_dataqueue_t s_dataqueue;

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Chip & App Info
    DRIVER_CHIPINFO_Init();
    DRIVER_APPINFO_Init();

    esp_chip_info_t c_info;
    uint32_t size_flash;
    uint32_t size_ram;
    uint8_t* buffer = (uint8_t*)malloc(512);
    util_dataqueue_item_t dq_i;

    size_flash = DRIVER_CHIPINFO_GetFlashSizeBytes();
    size_ram = DRIVER_CHIPINFO_GetRamSizeBytes();

    // Print App Information
    ESP_LOGI(DEBUG_TAG_MAIN, "-----------------------------------------------");
    memset(buffer, 0, 50);
    DRIVER_APPINFO_GetProjectName((char*)buffer);
    ESP_LOGI(DEBUG_TAG_MAIN, "PROJECT NAME : %s", (char*)buffer);
    memset(buffer, 0, 50);
    DRIVER_APPINFO_GetCompileDateTime((char*)buffer);
    ESP_LOGI(DEBUG_TAG_MAIN, "COMPILE DATETIME : %s", (char*)buffer);
    memset(buffer, 0, 50);
    DRIVER_APPINFO_GetIDFVersion((char*)buffer);
    ESP_LOGI(DEBUG_TAG_MAIN, "IDF VERSION : %s", (char*)buffer);
    memset(buffer, 0, 50);
    DRIVER_APPINFO_GetGitDetails((char*)buffer);
    ESP_LOGI(DEBUG_TAG_MAIN, "GIT DETAILS : %s", (char*)buffer);
    ESP_LOGI(DEBUG_TAG_MAIN, "-----------------------------------------------");

    memset(buffer, 0, 50);
    DRIVER_CHIPINFO_GetChipInfo(&c_info);
    DRIVER_CHIPINFO_GetChipID(buffer);

    // Print Chip Information
    ESP_LOGI(DEBUG_TAG_MAIN, "-----------------------------------------------");
    ESP_LOGI(DEBUG_TAG_MAIN, "MAC : "MACSTR, MAC2STR(buffer));
    ESP_LOGI(DEBUG_TAG_MAIN, "CHIP INFO: %s %s %s %s",
        CONFIG_IDF_TARGET,
        (c_info.features & CHIP_FEATURE_WIFI_BGN) ? "WIFI" : "",
        (c_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
        (c_info.features & CHIP_FEATURE_BT) ? "BT" : ""
    );
    ESP_LOGI(DEBUG_TAG_MAIN, "FLASH : %u MB", size_flash/(1024 * 1024));
    ESP_LOGI(DEBUG_TAG_MAIN, "-----------------------------------------------");

    // Set Internal Module Logging
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("esp_netif_handlers", ESP_LOG_NONE);
    esp_log_level_set("wifi_init", ESP_LOG_NONE);
    esp_log_level_set("phy_init", ESP_LOG_NONE);
    esp_log_level_set("pp", ESP_LOG_NONE);
    esp_log_level_set("net80211", ESP_LOG_NONE);

    // Main Code Starts
    ESP_LOGI(DEBUG_TAG_MAIN, "");
    ESP_LOGI(DEBUG_TAG_MAIN, "Init");
    ESP_LOGI(DEBUG_TAG_MAIN, "");

    // Initialize Spiffs & Filsystem
    memset((void*)buffer, 0, 512);
    DRIVER_SPIFFS_Init();
    DRIVER_SPIFFS_PrintInfo();
    if(DRIVER_SPIFFS_ReadFile("/spiff/hw_info.txt", (char*)buffer)){
        ESP_LOGI(DEBUG_TAG_MAIN, "%s", (char*)buffer);
    }else{
        ESP_LOGE(DEBUG_TAG_MAIN, "Hw Info Read Failed");
    }
    free(buffer);

    // Create Data Queue
    UTIL_DATAQUEUE_Create(&s_dataqueue, 4);
    
    // Intialize Drivers & Modules
    // Ensure All Other Peripherals Are Initialized Before Lcd So That Nothing Put Display DMA Out Of Sync
    DRIVER_LCD_Init();
    MODULE_LCD_Init();
    DRIVER_WIFI_Init();
    MODULE_WIFI_Init();
    DRIVER_API_Init();
    MODULE_API_Init();

    // Add Notification Targets
    MODULE_WIFI_AddNotificationTarget(&s_dataqueue);
    MODULE_API_AddNotificationTarget(&s_dataqueue);

    ESP_LOGI(DEBUG_TAG_MAIN, "Starting main task");

    // Start UI
    MODULE_LCD_StartUI();

    vTaskDelay(pdMS_TO_TICKS(250));
    
    // Set Location
    MODULE_LCD_SetLocation(DRIVER_API_WEATHER_CITYNAME","DRIVER_API_WEATHER_COUNTRYCODE);

    // Connect To Wifi
    dq_i.data_type = DATA_TYPE_COMMAND;
    dq_i.data = MODULE_WIFI_COMMAND_CONNECT;
    MODULE_WIFI_AddCommand(&dq_i);

    // Start Scheduler
    // No Need. ESP-IDF Automatically Starts The Scheduler Before main Is Called
    
    while(true)
    {
        if(UTIL_DATAQUEUE_MessageCheck(&s_dataqueue))
        {
            if(UTIL_DATAQUEUE_MessageGet(&s_dataqueue, &dq_i, 0))
            {
                ESP_LOGI(DEBUG_TAG_MAIN, "New In DataQueue. Type %u, Data %u", dq_i.data_type, dq_i.data);
                
                if(dq_i.data_type == DATA_TYPE_NOTIFICATION)
                {
                    switch(dq_i.data)
                    {
                        case DRIVER_WIFI_NOTIFICATION_GOT_IP:
                            MODULE_LCD_SetIP(dq_i.data_buff.value.ip);
                            break;
                        
                        case DRIVER_WIFI_NOTIFICATION_LOST_IP:
                        case DRIVER_WIFI_NOTIFICATION_DISCONNECTED:
                            MODULE_LCD_SetIP("????");
                            break;
                        
                        case DRIVER_API_NOTIFICATION_TIME_UPDATE:
                            MODULE_LCD_SetTime((driver_api_time_info_t *)&dq_i.data_buff.value.timedata);
                            break;

                        default:
                            break;
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelete(NULL);
}
