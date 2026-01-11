// MODULE_API
// JANUARY 1, 2026

#ifndef _MODULE_API_
#define _MODULE_API_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "util_dataqueue.h"

#define MODULE_API_EXECUTE_PERIOD_S         (30)
#define MODULE_API_DATAQUEUE_MAX            (4)
#define MODULE_API_NOTIFICATION_TARGET_MAX  (1)

typedef enum{
    MODULE_API_NOTIFICATION_TIME_UPDATE = 0,
    MODULE_API_NOTIFICATION_WEATHER_UPDATE,
}module_api_notification_type_t;

typedef enum{
    MODULE_API_STATE_IDLE = 0,
    MODULE_API_STATE_GET_TIME,
    MODULE_API_STATE_GET_WEATHER,
}module_api_state_t;

bool MODULE_API_Init(void);

bool MODULE_API_AddNotificationTarget(util_dataqueue_t* dq);

#endif