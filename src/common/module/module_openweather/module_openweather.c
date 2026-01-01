// MODULE_OPENWEATHER
// JANUARY 1, 2026
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "module_openweather.h"
#include "util_dataqueue.h"
#include "define_common_data_types.h"
#include "define_rtos_tasks.h"
#include "project_defines.h"

// Extern Variables
TaskHandle_t handle_task_module_openweather;

// Local Variables
static rtos_component_type_t s_component_type;

// Local Functions
static void s_task_function(void *pvParameters);

// External Functions
bool MODULE_OPENWEATHER_Init(void)
{
    // Initialize Module Openweather

    s_component_type = COMPONENT_TYPE_TASK;

    // Create Task
    xTaskCreate(
        s_task_function,
        "t-m-openwearher",
        TASK_STACK_DEPTH_MODULE_OPENWEATHER,
        NULL,
        TASK_PRIORITY_MODULE_OPENWEATHER,
        &s_task_handle
    );

    ESP_LOGI(DEBUG_TAG_MODULE_OPENWEATHER, "Type %u. Init", s_component_type);

    return true;
}

static void s_task_function(void *pvParameters)
{
    // Task Function

    util_dataqueue_item_t dq_i;

    ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "Starting task");

    while(true){
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelete(NULL);
}