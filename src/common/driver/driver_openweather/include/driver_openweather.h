// DRIVER_OPENWEATHER
// JANUARY 1, 2026

#ifndef _DRIVER_OPENWEATHER_
#define _DRIVER_OPENWEATHER_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#define DRIVER_OPENWEATHER_API_URL_FORMAT   "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric"
#define DRIVER_OPENWEATHER_API_CITYNAME     "Bengaluru"
#define DRIVER_OPENWEATHER_API_COUNTRYCODE  "IN"
#define DRIVER_OPENWEATHER_API_APIKEY       "302f94ec7416e369eea2c09309bfa098"

bool DRIVER_OPENWEATHER_Init(void);

bool DRIVER_OPENWEATHER_Get(void);

#endif