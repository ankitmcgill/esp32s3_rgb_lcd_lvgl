// DRIVER_API
// JANUARY 1, 2026

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"

#include "driver_api.h"
#include "define_common_data_types.h"
#include "define_rtos_tasks.h"
#include "project_defines.h"

#define DRIVER_API_API_URL_LEN_MAX      (128)
#define DRIVER_API_RESPONSE_LEN_MAX     (544)

// Extern Variables

// Local Variables
static rtos_component_type_t s_component_type;
static char* s_api_url;
static char* s_response;
static uint16_t s_response_len;

// Local Functions
static esp_err_t s_http_event_handler(esp_http_client_event_t *evt);

// External Functions
bool DRIVER_API_Init(void)
{
    // Initialize Driver API

    s_component_type = COMPONENT_TYPE_NON_TASK;

    // Allocate Buffers & Api Url
    s_response_len = 0;
    s_response = (char*)malloc(DRIVER_API_RESPONSE_LEN_MAX);
    assert(s_response);
    s_api_url = (char*)malloc(DRIVER_API_API_URL_LEN_MAX);
    assert(s_api_url);
    sprintf(s_api_url, 
        DRIVER_API_WEATHER_URL_FORMAT,
        DRIVER_API_WEATHER_CITYNAME,
        DRIVER_API_WEATHER_APIKEY
    );

    ESP_LOGI(DEBUG_TAG_DRIVER_API, "Type %u. Init", s_component_type);
    ESP_LOGI(DEBUG_TAG_DRIVER_API, 
        "City %s. Country %s", 
        DRIVER_API_WEATHER_CITYNAME,
        DRIVER_API_WEATHER_COUNTRYCODE
    );
    ESP_LOGI(DEBUG_TAG_DRIVER_API, "Api Url : %s", s_api_url);

    return true;
}

bool DRIVER_API_GetWeather(driver_api_weather_info_t* w_info)
{
    // Get Data

    esp_err_t err;
    esp_http_client_config_t config = {
        .url = s_api_url,
        .method = HTTP_METHOD_GET,
        .event_handler = s_http_event_handler,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    s_response_len = 0;
    memset(s_response, 0, DRIVER_API_RESPONSE_LEN_MAX);

    ESP_LOGI(DEBUG_TAG_DRIVER_API, "Sending GET request");

    err = esp_http_client_perform(client);
    if(err != ESP_OK){
        ESP_LOGE(DEBUG_TAG_DRIVER_API, "Request failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    ESP_LOGI(DEBUG_TAG_DRIVER_API, "Response: %s", s_response);

    // Response JSON Parsing
    // Weather
    cJSON* root = cJSON_Parse(s_response);
    if(!root){
        ESP_LOGE(DEBUG_TAG_DRIVER_API, "Invalid JSON");
        goto cleanup;
    }

    // Weather Array
    cJSON* weather = cJSON_GetObjectItem(root, "weather");
    if(cJSON_IsArray(weather)){
        cJSON* item = cJSON_GetArrayItem(weather, 0);
        if(cJSON_IsObject(item)){
            cJSON* id = cJSON_GetObjectItem(item, "id");
            cJSON* main = cJSON_GetObjectItem(item, "main");
            cJSON* desc = cJSON_GetObjectItem(item, "description");
            cJSON* icon = cJSON_GetObjectItem(item, "icon");

            if(cJSON_IsNumber(id) && cJSON_IsString(main) && cJSON_IsString(desc) && cJSON_IsString(icon)){
                ESP_LOGI(DEBUG_TAG_DRIVER_API, "id: %u", id->valueint);
                ESP_LOGI(DEBUG_TAG_DRIVER_API, "main: %s", main->valuestring);
                ESP_LOGI(DEBUG_TAG_DRIVER_API, "id: %s", desc->valuestring);
                ESP_LOGI(DEBUG_TAG_DRIVER_API, "icon: %s", icon->valuestring);

                w_info->weather_id = id->valueint;
                strcpy(w_info->weather_main, main->valuestring);
                strcpy(w_info->weather_description, desc->valuestring);
                strcpy(w_info->weather_icon, icon->valuestring);
            }
        }
    }

    // Main Weather Values
    cJSON* main = cJSON_GetObjectItem(root, "main");
    if(cJSON_IsObject(main)){
        cJSON *temp = cJSON_GetObjectItem(main, "temp");
        cJSON *humidity = cJSON_GetObjectItem(main, "humidity");

        if (cJSON_IsNumber(temp) && cJSON_IsNumber(humidity)){
            ESP_LOGI(DEBUG_TAG_DRIVER_API, "temperature: %.2f", temp->valuedouble);
            ESP_LOGI(DEBUG_TAG_DRIVER_API, "humidity: %d", humidity->valueint);

            w_info->temp = temp->valuedouble;
            w_info->humidity = humidity->valueint;
        }
    }

    // Sunrise / Sunset
    cJSON* sys = cJSON_GetObjectItem(root, "sys");
    if(cJSON_IsObject(sys)){
        cJSON *sunrise = cJSON_GetObjectItem(sys, "sunrise");
        cJSON *sunset = cJSON_GetObjectItem(sys, "sunset");

        if(cJSON_IsNumber(sunrise) && cJSON_IsNumber(sunset)){
            ESP_LOGI(DEBUG_TAG_DRIVER_API, "sunrise: %ld", (long)sunrise->valueint);
            ESP_LOGI(DEBUG_TAG_DRIVER_API, "sunset: %ld", (long)sunset->valueint);

            w_info->sunrise = (long)sunrise->valueint;
            w_info->sunset = (long)sunset->valueint;
        }
    }

    cJSON_Delete(root);
    esp_http_client_cleanup(client);
    return true;

    cleanup:
        esp_http_client_cleanup(client);
        ESP_LOGI(DEBUG_TAG_DRIVER_API, "HTTP task finished");
        return false;
}

static esp_err_t s_http_event_handler(esp_http_client_event_t *evt)
{
    // Http Event Handler

    switch(evt->event_id){
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)){
                if(s_response_len + evt->data_len < DRIVER_API_RESPONSE_LEN_MAX){
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