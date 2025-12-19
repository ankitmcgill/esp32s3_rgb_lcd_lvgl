// MODULE_WIFI
// SEPTEMBER 6, 2025

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "module_wifi.h"
#include "define_common_data_types.h"
#include "define_rtos_globals.h"
#include "define_rtos_tasks.h"
#include "project_defines.h"

// Extern Variables
TaskHandle_t handle_task_module_wifi;

// Local Variables
static module_wifi_state_t s_state;
static module_wifi_state_t s_state_prev;
static rtos_component_type_t s_component_type;
static message_queue_t s_message_queue;
static esp_timer_handle_t s_timer_handle;
static uint8_t s_retry_count;

// Local Functions
static void s_state_set(module_wifi_state_t newstate);
static void s_state_mainiter(void);
static void s_task_function(void *pvParameters);

// External Functions
bool MODULE_WIFI_Init(void)
{
    // Initialize Module Wifi

    s_component_type = COMPONENT_TYPE_TASK;
    s_state = -1;
    s_state_prev = -1;
    s_state_set(MODULE_WIFI_STATE_IDLE);

    // Create Incoming Message Queue
    s_message_queue.handle = xQueueCreate(MODULE_WIFI_MESSAGE_QUEUE_MAX, sizeof(message_item_t));
    s_message_queue.count = 0;

    // Create Task
    xTaskCreate(
        s_task_function,
        "t-m-wifi",
        TASK_STACK_DEPTH_MODULE_WIFI,
        NULL,
        TASK_PRIORITY_MODULE_WIFI,
        &handle_task_module_wifi
    );

    ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "Type %u. Init", s_component_type);

    return true;
}

bool MODULE_WIFI_SendMessage(module_wifi_message_type_t message, TickType_t wait)
{
    // Send Message

    if(s_message_queue.count >= MODULE_WIFI_MESSAGE_QUEUE_MAX){
        return false;
    }

    message_item_t message_item;
    message_item.message = message;
    BaseType_t ret = xQueueSend(s_message_queue.handle, (void*)&message_item, wait);
    s_message_queue.count += 1;

    return (ret == pdPASS);
}

static void s_state_set(module_wifi_state_t newstate)
{
    // Module Wifi Set State
    
    if(s_state == newstate){
        return;
    }

    s_state_prev = s_state;
    s_state = newstate;

    ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "%u -> %u", s_state_prev, s_state);
}

static void s_state_mainiter(void)
{
    // State Mainiter
    
    driver_wifi_message_type_t message;

    switch(s_state)
    {
        case MODULE_WIFI_STATE_IDLE:
            s_state_set(MODULE_WIFI_STATE_CHECK_SAVED_CREDENTIALS);
            break;

        case MODULE_WIFI_STATE_CHECK_SAVED_CREDENTIALS:
            if(DRIVER_WIFI_CheckSavedWifiCredentials())
            {
                ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "Saved Wi-Fi Credential Found");

                // WiFi Connect
            }

            // No Saved Credentials
            ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "No Saved Wi-Fi Credential Found");
            s_state_set(MODULE_WIFI_STATE_CHECK_DEFAULT_CREDENTIALS);
            break;

        case MODULE_WIFI_STATE_CHECK_DEFAULT_CREDENTIALS:
            if(defined(DEFAULT_WIFI_SSID) && DEFAULT_WIFI_SSID != ""){
                if(defined(DEFAULT_WIFI_PASSWORD) && DEFAULT_WIFI_PASSWORD != ""){
                    ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "Default Wi-Fi Credential Found");
                    ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "SSID: %s", DEFAULT_WIFI_SSID);
                    ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "Password: %s", DEFAULT_WIFI_PASSWORD);

                    // WiFi Connect
                }
            }

            // No Default Credentials
            ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "No Default Wi-Fi Credential Found");
            s_state_set(MODULE_WIFI_STATE_SCAN);
            break;

        case MODULE_WIFI_STATE_SCAN:
            message = DRIVER_WIFI_MESSAGE_SCAN;
            DRIVER_WIFI_SendMessage(message, 0);
            s_state_set(MODULE_WIFI_STATE_SCANNING);
            break;
        
        case MODULE_WIFI_STATE_SCANNING:
            // Do Nothing
            break;
        
        case MODULE_WIFI_STATE_SCAN_DONE:
            s_state_set(MODULE_WIFI_STATE_SMARTCONFIG);
            break;

        case MODULE_WIFI_STATE_SMARTCONFIG:
        case MODULE_WIFI_STATE_CONNECT:
        case MODULE_WIFI_STATE_CONNECTING:
        case MODULE_WIFI_STATE_CONNECTED:
        case MODULE_WIFI_STATE_GOT_IP:
        case MODULE_WIFI_STATE_LOST_IP:
        case MODULE_WIFI_STATE_DISCONNECTED:
            break;
    }
}

static void s_task_function(void *pvParameters)
{
    // Task Function

    BaseType_t ret;
    message_item_t message_item;
    notification_item_t notification_item;

    ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "Starting task");

    while(true){
        // Check For Incoming Messages
        if(s_message_queue.count != 0)
        {
            ret = xQueueReceive(s_message_queue.handle, (void*)&message_item, 0);
            if(ret == pdPASS){
                ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "New Message Received. Type : %u", message_item.message);
                
                s_message_queue.count -= 1;

                switch(message_item.message)
                {
                    case MODULE_WIFI_MESSAGE_CONNECT:
                        s_state_set(MODULE_WIFI_MESSAGE_CONNECT);
                        break;

                    default:
                        break;
                }
            }
        }

        // Check For Notifications
        if(DRIVER_WIFI_CheckNotification((driver_wifi_notification_type_t*)&notification_item.notification, 0)){

        }

        // Run State Mainiter
        s_state_mainiter();

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelete(NULL);
}

//         switch(s_state)
//         {
//             case MODULE_WIFI_STATE_IDLE:
//                 s_state_set(MODULE_WIFI_CHECK_PROVISIONED);
//                 break;

//             case MDOULE_WIFI_CHECK_PROVISIONED:
//                 s_state_set(MODULE_WIFI_STATE_CONNECT_DEFAULT);
//                 break;
            
//             case MODULE_WIFI_STATE_CONNECT_DEFAULT:
//                 #if defined(DEFAULT_WIFI_SSID) && defined(DEFAULT_WIFI_PASSWORD)
//                     if(strlen(DEFAULT_WIFI_SSID)!=0 && strlen(DEFAULT_WIFI_PASSWORD)!=0)
//                     {
//                         ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "Connecting To Default Wifi: %s", DEFAULT_WIFI_SSID);
                        
//                         DRIVER_WIFI_Connect(
//                             DEFAULT_WIFI_SSID,
//                             DEFAULT_WIFI_PASSWORD
//                         );
//                         s_set_state(MODULE_WIFI_STATE_CONNECTING);
//                     }
//                     break;
//                 #endif

//                 // Default Wifi SSID Or Password Not Avaialable
//                 s_state_set(MODULE_WIFI_STATE_PROVISION_SMARTCONFIG);
//                 break;

//             case MODULE_WIFI_STATE_PROVISION_SMARTCONFIG:
//                 break;
            
//             case MODULE_WIFI_STATE_CONNECTING:
//                 ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "WiFi Connecting...");
//                 break;
            
//             case MODULE_WIFI_STATE_CONNECTED:
//                 ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "WiFi Connected");
//                 break;
            
//             case MODULE_WIFI_STATE_DISCONNECTED:
//                 ESP_LOGI(DEBUG_TAG_MODULE_WIFI, "WiFi Connected");
                
//                 // Re-start Wifi Connection Cycle
//                 s_state_set(MODULE_WIFI_STATE_IDLE);
//                 break;

//             default:
//                 break;
//         }

        
//     };

//     vTaskDelete(NULL);
// }