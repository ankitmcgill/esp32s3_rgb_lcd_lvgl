// MODULE_LCD
// OCTOBER 4, 2025

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "module_lcd.h"
#include "define_common_data_types.h"
#include "define_rtos_globals.h"
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

    DRIVER_LCD_Demo();

    ESP_LOGI(DEBUG_TAG_MODULE_LCD, "Type %u. Init", s_component_type);

    return true;
}

static void s_task_function(void *pvParameters)
{
    // Task Function

    while(true){
        vTaskDelay(pdMS_TO_TICKS(500));
    };

    vTaskDelete(NULL);
}