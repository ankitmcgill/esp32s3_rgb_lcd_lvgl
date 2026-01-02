// DRIVER_API
// JANUARY 1, 2026

#ifndef _DRIVER_API_
#define _DRIVER_API_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

// Weather Api - Openweather (https://home.openweathermap.org/)
#define DRIVER_API_WEATHER_URL_FORMAT   "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric"
#define DRIVER_API_WEATHER_CITYNAME     "Bengaluru"
#define DRIVER_API_WEATHER_COUNTRYCODE  "IN"
#define DRIVER_API_WEATHER_APIKEY       "302f94ec7416e369eea2c09309bfa098"

// Time Api - Timezonedb (https://timezonedb.com/)
#define DRIVER_API_TIME_URL_FORMAT  "http://api.timezonedb.com/v2.1/get-time-zone?key=%s&format=json&by=zone&zone=%s"
#define DRIVER_API_TIME_ZONE        "Asia/Kolkata"
#define DRIVER_API_TIME_APIKEY      "EQ26WOQQQD91"

typedef struct{
    uint16_t weather_id;
    char weather_main[16];
    char weather_description[32];
    char weather_icon[8];
    float temp;
    float humidity;
    uint32_t sunrise;
    uint32_t sunset;
}driver_api_weather_info_t;

typedef struct{
    uint32_t timestamp;
    char time_string[16];
    char am_pm_string[3];
    char date_string[48];
}driver_api_time_info_t;

bool DRIVER_API_Init(void);

bool DRIVER_API_GetWeather(driver_api_weather_info_t* w_info);
bool DRIVER_API_GetTime(driver_api_time_info_t* t_info);

#endif