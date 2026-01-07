// MODULE_LCD
// OCTOBER 4, 2025

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "module_lcd.h"
#include "driver_lcd.h"
#include "util_dataqueue.h"
#include "define_common_data_types.h"
#include "define_rtos_tasks.h"
#include "project_defines.h"

// Extern Variables

// Local Variables
static rtos_component_type_t s_component_type;
static TaskHandle_t s_task_handle;

// Local Functions
static void s_task_function(void *pvParameters);

// External Functions
bool MODULE_LCD_Init(void)
{
    // Initialize Module Lcd

    s_component_type = COMPONENT_TYPE_TASK;

    // Create Task
    xTaskCreate(
        s_task_function,
        "t-m-lcd",
        TASK_STACK_DEPTH_MODULE_LCD,
        NULL,
        TASK_PRIORITY_MODULE_LCD,
        &s_task_handle
    );

    ESP_LOGI(DEBUG_TAG_MODULE_LCD, "Type %u. Init", s_component_type);

    return true;
}

bool MODULE_LCD_StartUI(void)
{
    // Start UI

    util_dataqueue_item_t dq_i = {
        .data_type = DATA_TYPE_COMMAND,
        .data = DRIVER_LCD_COMMAND_LOAD_UI
    };
    return DRIVER_LCD_AddCommand(&dq_i);
}

bool MODULE_LCD_Demo(void)
{
    // Lcd Demo

    util_dataqueue_item_t dq_i = {
        .data_type = DATA_TYPE_COMMAND,
        .data = DRIVER_LCD_COMMAND_DEMO
    };
    return DRIVER_LCD_AddCommand(&dq_i);
}

bool MODULE_LCD_SetIP(char* ip)
{
    // Set IP Address Field

    util_dataqueue_item_t dq_i = {
        .data_type = DATA_TYPE_COMMAND,
        .data = DRIVER_LCD_COMMAND_SET_IP
    };
    strcpy(dq_i.data_buff.value.ip, ip);
    return DRIVER_LCD_AddCommand(&dq_i);
}

bool MODULE_LCD_SetTime(driver_api_time_info_t* ti)
{
    // Set Time Fields

    util_dataqueue_item_t dq_i = {
        .data_type = DATA_TYPE_COMMAND,
        .data = DRIVER_LCD_COMMAND_SET_TIME
    };
    memcpy(&dq_i.data_buff.value.timedata, ti, sizeof(driver_api_time_info_t));
    return DRIVER_LCD_AddCommand(&dq_i);
}

static void s_task_function(void *pvParameters)
{
    // Task Function

    while(true){
        vTaskDelay(pdMS_TO_TICKS(500));
    };

    vTaskDelete(NULL);
}
