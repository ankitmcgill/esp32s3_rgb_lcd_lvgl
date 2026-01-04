// MODULE_API
// JANUARY 1, 2026

#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "module_api.h"
#include "driver_api.h"
#include "driver_wifi.h"
#include "define_common_data_types.h"
#include "define_rtos_tasks.h"
#include "project_defines.h"

// Extern Variables
TaskHandle_t handle_task_module_api;

// Local Variables
static rtos_component_type_t s_component_type;
static util_dataqueue_t s_dataqueue;
static esp_timer_handle_t s_timer;
static driver_api_weather_info_t s_info_weather;
static driver_api_time_info_t s_info_time;

// Local Functions
static void s_task_function(void *pvParameters);
static void s_timer_cb(void *arg);

// External Functions
bool MODULE_API_Init(void)
{
    // Initialize Module Api

    s_component_type = COMPONENT_TYPE_TASK;

    // Create Data Queue
    UTIL_DATAQUEUE_Create(&s_dataqueue, MODULE_API_DATAQUEUE_MAX);

    // Create Timer
    const esp_timer_create_args_t timer_args = {
        .callback = &s_timer_cb,
        .name = "periodic_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_timer));

    // Add Notification Targets
    DRIVER_WIFI_AddNotificationTarget(&s_dataqueue);

    // Create Task
    xTaskCreate(
        s_task_function,
        "t-m-api",
        TASK_STACK_DEPTH_MODULE_API,
        NULL,
        TASK_PRIORITY_MODULE_API,
        &handle_task_module_api
    );

    ESP_LOGI(DEBUG_TAG_MODULE_API, "Type %u. Init", s_component_type);

    return true;
}

static void s_task_function(void *pvParameters)
{
    // Task Function

    util_dataqueue_item_t dq_i;

    ESP_LOGI(DEBUG_TAG_MODULE_API, "Starting task");

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
                    switch(dq_i.data){
                        case DRIVER_WIFI_NOTIFICATION_GOT_IP:
                            ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "Wifi connected. Starting periodic api @ %us", MODULE_API_EXECUTE_PERIOD_S);

                            // Run Once Immidiately
                            s_timer_cb(NULL);

                            // Start Timer
                            ESP_ERROR_CHECK(esp_timer_start_periodic(s_timer, MODULE_API_EXECUTE_PERIOD_S * 1000 * 1000));
                            break;
                    
                        case DRIVER_WIFI_NOTIFICATION_LOST_IP:
                        case DRIVER_WIFI_NOTIFICATION_DISCONNECTED:
                            ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "Wifi disconnected. Stopping periodic api");

                            ESP_ERROR_CHECK(esp_timer_stop(s_timer));
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

static void s_timer_cb(void *arg)
{
    // Get Weather
    if(!DRIVER_API_GetWeather(&s_info_weather)){
        ESP_LOGE(DEBUG_TAG_MODULE_WIFI, "Weather api fail");
    }

    // Get Time
    if(!DRIVER_API_GetTime(&s_info_time)){
        ESP_LOGE(DEBUG_TAG_MODULE_WIFI, "Weather api fail");
    }
}