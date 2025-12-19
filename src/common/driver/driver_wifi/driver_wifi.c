// DRIVER_WIFI
// SEPTEMBER 6, 2025

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_smartconfig.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver_wifi.h"
#include "define_common_data_types.h"
#include "define_rtos_globals.h"
#include "define_rtos_tasks.h"

// Extern Variables
TaskHandle_t handle_task_driver_wifi;

// Local Variables
static rtos_component_type_t s_component_type;
static message_queue_t s_message_queue;
static notification_queue_t s_notification_queue;
static char s_ssid[DRIVER_WIFI_LEN_SSID_MAX];
static char s_password[DRIVER_WIFI_LED_PWD_MAX];
static uint16_t s_scan_ap_count;
static wifi_ap_record_t* s_scan_ap_records;

// Local Functions
static void s_task_function(void *pvParameters);

// static void s_send_change_notification(void);
static void s_wifi_connect(void);
static void s_wifi_disconnect(void);
static bool s_wifi_notify(notification_item_t* item, TickType_t wait);
static void s_task_function(void *pvParameters);
static void s_event_handler_wifi(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void s_event_handler_smartconfig(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

// External Functions
bool DRIVER_WIFI_Init(void)
{
    // Initialize Driver Wifi

    // Create Incoming Message Queue
    s_message_queue.handle = xQueueCreate(DRIVER_WIFI_MESSAGE_QUEUE_MAX, sizeof(message_item_t));
    s_message_queue.count = 0;

    // Create Outgoing Notification Queue
    s_notification_queue.handle = xQueueCreate(DRIVER_WIFI_NOTIFICATION_QUEUE_MAX, sizeof(notification_item_t));
    s_notification_queue.count = 0;

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_event_handler_instance_t event_handler_got_ip;
    esp_event_handler_instance_t event_handler_any_event;

    s_component_type = COMPONENT_TYPE_TASK;

    // Allocate Space For Scan AP Records
    s_scan_ap_records = (wifi_ap_record_t*)malloc(DRIVER_WIFI_SCAN_RESULTS_COUNT_MAX * sizeof(wifi_ap_record_t));
    assert(s_scan_ap_records);

    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_wifi_set_storage(WIFI_STORAGE_FLASH);

    // Initialize WiFi And TcpIP
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

    // Start In Station Mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Register Event Handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &s_event_handler_wifi,
        NULL,
        &event_handler_any_event
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP | IP_EVENT_STA_LOST_IP,
        &s_event_handler_wifi,
        NULL,
        &event_handler_got_ip
    ));
    ESP_ERROR_CHECK(esp_event_handler_register(
        SC_EVENT, 
        ESP_EVENT_ANY_ID, 
        &s_event_handler_smartconfig, 
        NULL
    ));

    // Create Driver Wifi Task
    xTaskCreate(
        s_task_function,
        "t-d-wifi",
        TASK_STACK_DEPTH_DRIVER_WIFI,
        NULL,
        TASK_PRIORITY_DRIVER_WIFI,
        &handle_task_driver_wifi
    );

    ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Type %u. Init", s_component_type);

    return true;
}

bool DRIVER_WIFI_SendMessage(driver_wifi_message_type_t message, TickType_t wait)
{
    // Send Message

    if(s_message_queue.count >= DRIVER_WIFI_MESSAGE_QUEUE_MAX){
        return false;
    }

    message_item_t message_item;
    message_item.message = message;
    BaseType_t ret = xQueueSend(s_message_queue.handle, (void*)&message_item, wait);
    s_message_queue.count += 1;

    return (ret == pdPASS);
}

bool DRIVER_WIFI_CheckNotification(driver_wifi_notification_type_t* retval, TickType_t wait)
{
    // Check Notification
    notification_item_t notification_item;
    BaseType_t ret;

    if(s_notification_queue.count == 0){
        return false;
    }

    ret = xQueueReceive(s_notification_queue.handle, (void*)&notification_item, 0);
    if(ret != pdPASS){
        return false;
    }

    *retval = notification_item.notification;
    notification_item.viewer_count -= 1;
    if(notification_item.viewer_count == 0){
        // Re-insert Notification Back Into Queue
        ret = xQueueSend(s_notification_queue.handle, (void*)&notification_item, wait);
    }

    return true;
}

bool DRIVER_WIFI_CheckSavedWifiCredentials(void)
{
    // Check If Saved Wifi Credentials Are Present

    wifi_config_t w_config;

    esp_wifi_get_config(WIFI_IF_STA, &w_config);
    return (w_config.sta.ssid[0] != '\0');
}

void DRIVER_WIFI_Connect(char* ssid, char* password)
{
    // Connect Using Supplied Credentials
    // If NULL, Connect Using Saved Wifi Credentials

    if(ssid && password)
    {
        memcpy(s_ssid, (void*)ssid, DRIVER_WIFI_LEN_SSID_MAX);
        memcpy(s_password, (void*)password, DRIVER_WIFI_LED_PWD_MAX);
    }
    
}

static void s_wifi_connect(void)
{
    // Connect Wifi

    wifi_config_t w_config;

    // Assumes Target Wifi Credentials Are Present In s_ssid & s_password
    strcpy((char*)w_config.sta.ssid, s_ssid);
    strcpy((char*)w_config.sta.password, s_password);
    w_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    w_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Connecting...");

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &w_config));
    esp_wifi_connect();
}

static void s_wifi_disconnect(void)
{
    // Disconnect Wifi

    ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Disconnecting...");

    esp_wifi_disconnect();
}

static bool s_wifi_notify(notification_item_t* item, TickType_t wait)
{
    //Add Notification Item

    if(s_notification_queue.count >= DRIVER_WIFI_NOTIFICATION_QUEUE_MAX){
        return false;
    }

    notification_item_t notification_item;
    notification_item.notification = item->notification;
    notification_item.viewer_count = item->viewer_count;
    BaseType_t ret = xQueueSend(s_notification_queue.handle, (void*)&notification_item, wait);
    s_notification_queue.count += 1;

    return (ret == pdPASS);
}

static void s_task_function(void *pvParameters)
{
    // Task Function

    BaseType_t ret;
    message_item_t message_item;
    wifi_scan_config_t wifi_scan_config;

    ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Starting task");

    while(true){
        // Check For Incoming Messages
        if(s_message_queue.count != 0)
        {
            ret = xQueueReceive(s_message_queue.handle, (void*)&message_item, 0);
            if(ret == pdPASS){
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "New Message Received. Type : %u", message_item.message);
                
                s_message_queue.count -= 1;

                switch(message_item.message)
                {
                    case DRIVER_WIFI_MESSAGE_SCAN:
                        wifi_scan_config.ssid = NULL;
                        wifi_scan_config.bssid = NULL;
                        wifi_scan_config.channel = 0;
                        wifi_scan_config.show_hidden = false;
                        wifi_scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE,
                        wifi_scan_config.scan_time.active.min = 100;
                        wifi_scan_config.scan_time.active.max = 300;

                        ESP_ERROR_CHECK(esp_wifi_scan_start(&wifi_scan_config, false));
                        break;

                    case DRIVER_WIFI_MESSAGE_CONNECT:
                        s_wifi_connect();
                        break;
                    
                    case DRIVER_WIFI_MESSAGE_DISCONNECT:
                        s_wifi_disconnect();
                    
                    default:
                        break;
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    };

    vTaskDelete(NULL);
}

static void s_event_handler_wifi(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    // Wifi Event Handler

    notification_item_t notification_item;

    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - WIFI_EVENT_STA_START");
                break;
            
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - WIFI_EVENT_STA_CONNECTED");

                notification_item.notification = DRIVER_WIFI_NOTIFICATION_CONNECTED;
                notification_item.viewer_count = 1;
                s_wifi_notify(&notification_item, 0);
                break;
            
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - WIFI_EVENT_STA_DISCONNECTED");

                notification_item.notification = DRIVER_WIFI_NOTIFICATION_DISCONNECTED;
                notification_item.viewer_count = 1;
                s_wifi_notify(&notification_item, 0);
                break;
            
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - WIFI_EVENT_SCAN_DONE");

                s_scan_ap_count = DRIVER_WIFI_SCAN_RESULTS_COUNT_MAX;
                ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&s_scan_ap_count, s_scan_ap_records));
                // ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&s_scan_ap_count));
                
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "%u AP Found", s_scan_ap_count);

                for (int i = 0; i < s_scan_ap_count; i++) {
                    ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "[%02d] SSID: %-32s | RSSI: %d | Channel: %d | Auth: %d",
                        i + 1,
                        s_scan_ap_records[i].ssid,
                        s_scan_ap_records[i].rssi,
                        s_scan_ap_records[i].primary,
                        s_scan_ap_records[i].authmode);
                };

                notification_item.notification = DRIVER_WIFI_NOTIFICATION_SCAN_DONE;
                notification_item.viewer_count = 1;
                s_wifi_notify(&notification_item, 0);
                break;
            
            default:
                break;
        }

        return;
    }

    if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - IP_EVENT_STA_GOT_IP");
                
                ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - Got IP : " IPSTR, IP2STR(&(event->ip_info.ip)));

                notification_item.notification = DRIVER_WIFI_NOTIFICATION_GOT_IP;
                notification_item.viewer_count = 1;
                s_wifi_notify(&notification_item, 0);
                break;
            
            case IP_EVENT_STA_LOST_IP:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - IP_EVENT_STA_LOST_IP");

                notification_item.notification = DRIVER_WIFI_NOTIFICATION_LOST_IP;
                notification_item.viewer_count = 1;
                s_wifi_notify(&notification_item, 0);
                break;
            
            default:
                break;
        }
        
        return;
    }
}

static void s_event_handler_smartconfig(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Smartconfig Event Handler

    if(event_base == SC_EVENT)
    {
        switch(event_id)
        {
            case SC_EVENT_SCAN_DONE:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - SC_EVENT_SCAN_DONE");
                break;
            
            case SC_EVENT_FOUND_CHANNEL:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - SC_EVENT_FOUND_CHANNEL");
                break;
            
            case SC_EVENT_GOT_SSID_PSWD:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - SC_EVENT_GOT_SSID_PSWD");

                smartconfig_event_got_ssid_pswd_t *evt = event_data;
                memset(s_ssid, 0, 32);
                memset(s_password, 0, 64);
                memcpy(s_ssid, evt->ssid, 32);
                memcpy(s_password, evt->password, 64);

                break;
            
            case SC_EVENT_SEND_ACK_DONE:
                ESP_LOGI(DEBUG_TAG_DRIVER_WIFI, "Event - SC_EVENT_SEND_ACK_DONE");
                
                // Stop SmartConfig
                esp_smartconfig_stop();
                break;
            
            default:
                break;
        }

        return;
    }
}