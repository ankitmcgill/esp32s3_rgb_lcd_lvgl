// MODULE_OPENWEATHER
// JANUARY 1, 2026

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "module_openweather.h"
#include "define_common_data_types.h"
#include "define_rtos_tasks.h"
#include "project_defines.h"

// Extern Variables
TaskHandle_t handle_task_module_openweather;

// Local Variables
static rtos_component_type_t s_component_type;
static util_dataqueue_t s_dataqueue;

// Local Functions
static void s_task_function(void *pvParameters);

// External Functions
bool MODULE_OPENWEATHER_Init(void)
{
    // Initialize Module Wifi

    s_component_type = COMPONENT_TYPE_TASK;

    // Create Data Queue
    UTIL_DATAQUEUE_Create(&s_dataqueue, MODULE_OPENWEATHER_DATAQUEUE_MAX);

    // Create Task
    xTaskCreate(
        s_task_function,
        "t-m-openweather",
        TASK_STACK_DEPTH_MODULE_OPENWEATHER,
        NULL,
        TASK_PRIORITY_MODULE_OPENWEATHER,
        &handle_task_module_openweather
    );

    ESP_LOGI(DEBUG_TAG_MODULE_OPENWEATHER, "Type %u. Init", s_component_type);

    return true;
}

bool MODULE_WIFI_AddCommand(util_dataqueue_item_t* dq_i)
{
    // Add Command

    return UTIL_DATAQUEUE_MessageQueue(&s_dataqueue, dq_i, 0);
}

static void s_task_function(void *pvParameters)
{
    // Task Function

    util_dataqueue_item_t dq_i;

    ESP_LOGI(DEBUG_TAG_MODULE_OPENWEATHER, "Starting task");

    while(true){
        // Check Data Queue
        if(UTIL_DATAQUEUE_MessageCheck(&s_dataqueue))
        {
            if(UTIL_DATAQUEUE_MessageGet(&s_dataqueue, &dq_i, 0))
            {
                ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "New In DataQueue. Type %u, Data %u", dq_i.data_type, dq_i.data);
                
                if(dq_i.data_type == DATA_TYPE_COMMAND)
                {
                    
                }
                else if(dq_i.data_type == DATA_TYPE_NOTIFICATION)
                {

                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelete(NULL);
}