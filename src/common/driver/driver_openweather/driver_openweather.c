// DRIVER_OPENWEATHER
// JANUARY 1, 2026

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"

#include "driver_openweather.h"
#include "define_common_data_types.h"
#include "define_rtos_tasks.h"
#include "project_defines.h"

#define DRIVER_OPENWEATHER_API_URL_LEN_MAX      (128)
#define DRIVER_OPENWEATHER_RESPONSE_LEN_MAX     (544)

// Extern Variables

// Local Variables
static rtos_component_type_t s_component_type;
static char* s_api_url;
static char* s_response;
static uint16_t s_response_len;

// Local Functions
static esp_err_t s_http_event_handler(esp_http_client_event_t *evt);

// External Functions
bool DRIVER_OPENWEATHER_Init(void)
{
    // Initialize Driver Openweather

    s_component_type = COMPONENT_TYPE_NON_TASK;

    // Allocate Buffers & Api Url
    s_response_len = 0;
    s_response = (char*)malloc(DRIVER_OPENWEATHER_RESPONSE_LEN_MAX);
    assert(s_response);
    s_api_url = (char*)malloc(DRIVER_OPENWEATHER_API_URL_LEN_MAX);
    assert(s_api_url);
    sprintf(s_api_url, 
        DRIVER_OPENWEATHER_API_URL_FORMAT,
        DRIVER_OPENWEATHER_API_CITYNAME,
        DRIVER_OPENWEATHER_API_APIKEY
    );

    ESP_LOGI(DEBUG_TAG_DRIVER_OPENWEATHER, "Type %u. Init", s_component_type);
    ESP_LOGI(DEBUG_TAG_DRIVER_OPENWEATHER, 
        "City %s. Country %s", 
        DRIVER_OPENWEATHER_API_CITYNAME,
        DRIVER_OPENWEATHER_API_COUNTRYCODE
    );
    ESP_LOGI(DEBUG_TAG_DRIVER_OPENWEATHER, "Api Url : %s", s_api_url);

    return true;
}

bool DRIVER_OPENWEATHER_Get(void)
{
    // Get Data

    esp_err_t err;
    s_response_len = 0;
    memset(s_response, 0, DRIVER_OPENWEATHER_RESPONSE_LEN_MAX);
    
    esp_http_client_config_t config = {
        .url = s_api_url,
        .method = HTTP_METHOD_GET,
        .event_handler = s_http_event_handler,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    ESP_LOGI(DEBUG_TAG_DRIVER_OPENWEATHER, "Sending GET request");

    err = esp_http_client_perform(client);
    if(err != ESP_OK){
        ESP_LOGE(DEBUG_TAG_DRIVER_OPENWEATHER, "Request failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    ESP_LOGI(DEBUG_TAG_DRIVER_OPENWEATHER, "Response: %s", s_response);

    // Response JSON Parsing
    cJSON *json = cJSON_Parse(s_response);
    if (!json){
        ESP_LOGE(DEBUG_TAG_DRIVER_OPENWEATHER, "Invalid JSON");
        goto cleanup;
    }

    cJSON *status = cJSON_GetObjectItem(json, "status");
    cJSON *value  = cJSON_GetObjectItem(json, "value");

    if (cJSON_IsString(status)) {
        ESP_LOGI(DEBUG_TAG_DRIVER_OPENWEATHER, "Status: %s", status->valuestring);
    }

    if (cJSON_IsNumber(value)) {
        ESP_LOGI(DEBUG_TAG_DRIVER_OPENWEATHER, "Value: %d", value->valueint);
    }

    cJSON_Delete(json);

    cleanup:
        esp_http_client_cleanup(client);
        ESP_LOGI(DEBUG_TAG_DRIVER_OPENWEATHER, "HTTP task finished");
        
        return false;
}

static esp_err_t s_http_event_handler(esp_http_client_event_t *evt)
{
    // Http Event Handler

    switch(evt->event_id){
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)){
                if(s_response_len + evt->data_len < DRIVER_OPENWEATHER_RESPONSE_LEN_MAX){
                    memcpy(s_response + s_response_len, evt->data, evt->data_len);
                    s_response_len += evt->data_len;
                }
            }
            break;
        
        default:
            break;
    };

    return ESP_OK;
}