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
static module_api_state_t s_state;
static module_api_state_t s_state_prev;
static rtos_component_type_t s_component_type;
static util_dataqueue_t s_dataqueue;
static uint8_t s_notification_targets_count;
static util_dataqueue_t* s_notification_targets[MODULE_API_NOTIFICATION_TARGET_MAX];
static esp_timer_handle_t s_timer;
static driver_api_weather_info_t s_info_weather;
static driver_api_time_info_t s_info_time;

// Local Functions
static void s_state_set(module_api_state_t newstate);
static void s_state_mainiter(void);
static bool s_notify(util_dataqueue_item_t* dq_i, TickType_t wait);
static void s_task_function(void *pvParameters);
static void s_timer_cb(void *arg);

// External Functions
bool MODULE_API_Init(void)
{
    // Initialize Module Api

    s_component_type = COMPONENT_TYPE_TASK;
    s_state = -1;
    s_state_prev = -1;
    s_state_set(MODULE_API_STATE_IDLE);

    // Create Data Queue
    UTIL_DATAQUEUE_Create(&s_dataqueue, MODULE_API_DATAQUEUE_MAX);
    s_notification_targets_count = 0;

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

bool MODULE_API_AddNotificationTarget(util_dataqueue_t* dq)
{
    // Add Notification Target

    if(s_notification_targets_count >= MODULE_API_NOTIFICATION_TARGET_MAX){
        return false;
    }

    s_notification_targets[s_notification_targets_count] = dq;
    s_notification_targets_count += 1;

    return true;
}

static void s_state_set(module_api_state_t newstate)
{
    // Module Api Set State
    
    if(s_state == newstate){
        return;
    }

    s_state_prev = s_state;
    s_state = newstate;

    ESP_LOGI(DEBUG_TAG_MODULE_API, "%u -> %u", s_state_prev, s_state);
}

static void s_state_mainiter(void)
{
    // State Mainiter
    
    util_dataqueue_item_t dq_i;
    
    switch(s_state)
    {
        case MODULE_API_STATE_IDLE:
            // Do Nothing
            break;

        case MODULE_API_STATE_GET_TIME:
            if(!DRIVER_API_GetTime(&s_info_time)){
                ESP_LOGW(DEBUG_TAG_MODULE_API, "Time api fail");
            }

            // Send Notification
            dq_i.data_type = DATA_TYPE_NOTIFICATION;
            dq_i.data = MODULE_API_NOTIFICATION_TIME_UPDATE;
            dq_i.data_buff.value.timedata.timestamp = s_info_time.timestamp;
            strcpy(dq_i.data_buff.value.timedata.time_string, s_info_time.time_string);
            strcpy(dq_i.data_buff.value.timedata.am_pm_string, s_info_time.am_pm_string);
            strcpy(dq_i.data_buff.value.timedata.date_string, s_info_time.date_string);
            s_notify(&dq_i, 0);

            s_state_set(MODULE_API_STATE_GET_WEATHER);
            break;

        case MODULE_API_STATE_GET_WEATHER:
            if(!DRIVER_API_GetWeather(&s_info_weather)){
                ESP_LOGW(DEBUG_TAG_MODULE_API, "Weather api fail");
            }

            // Send Notification
            dq_i.data_type = DATA_TYPE_NOTIFICATION;
            dq_i.data = MODULE_API_NOTIFICATION_WEATHER_UPDATE;
            sprintf(dq_i.data_buff.value.weatherdata.humidity, "%d %%", s_info_weather.humidity);
            sprintf(dq_i.data_buff.value.weatherdata.temp, "%.0f C", s_info_weather.temp);
            s_notify(&dq_i, 0);

            s_state_set(MODULE_API_STATE_IDLE);
            break;
        
        default:
            break;
    }
}

static bool s_notify(util_dataqueue_item_t* dq_i, TickType_t wait)
{
    // Send Notification

    for(uint8_t i = 0; i < s_notification_targets_count; i++){
        if(!UTIL_DATAQUEUE_MessageQueue(s_notification_targets[i], dq_i, wait)){
            ESP_LOGW(DEBUG_TAG_MODULE_API, "Message Queue Failed %s", __FILE__);
        }
    }

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
                ESP_LOGI(DEBUG_TAG_MODULE_API, "New In DataQueue. Type %u, Data %u", dq_i.data_type, dq_i.data);
                
                if(dq_i.data_type == DATA_TYPE_COMMAND)
                {
                    
                }
                else if(dq_i.data_type == DATA_TYPE_NOTIFICATION)
                {
                    switch(dq_i.data){
                        case DRIVER_WIFI_NOTIFICATION_GOT_IP:
                            ESP_LOGI(DEBUG_TAG_MODULE_API, "Wifi connected. Starting periodic api @ %us", MODULE_API_EXECUTE_PERIOD_S);

                            // Run Once Immidiately
                            s_timer_cb(NULL);

                            // Start Timer
                            ESP_ERROR_CHECK(esp_timer_start_periodic(s_timer, MODULE_API_EXECUTE_PERIOD_S * 1000 * 1000));
                            break;
                    
                        case DRIVER_WIFI_NOTIFICATION_LOST_IP:
                        case DRIVER_WIFI_NOTIFICATION_DISCONNECTED:
                            ESP_LOGI(DEBUG_TAG_MODULE_API, "Wifi disconnected. Stopping periodic api");

                            if(esp_timer_is_active(s_timer)){
                                ESP_ERROR_CHECK(esp_timer_stop(s_timer));
                            }
                            break;
                        
                        default:
                            break;
                    }
                }
            }
        }

        // Run State Mainiter
        s_state_mainiter();

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelete(NULL);
}

static void s_timer_cb(void *arg)
{
    s_state_set(MODULE_API_STATE_GET_TIME);
}