// DEFINE COMMON DATA TYPES
// SEPTEMBER 9, 2025

#ifndef _DEFINE_COMMON_DATA_TYPES_
#define _DEFINE_COMMON_DATA_TYPES_

#include "driver_wifi.h"

typedef struct
{
    uint8_t message;
}message_item_t;
typedef struct
{
    QueueHandle_t handle;
    uint8_t count;
}message_queue_t;

typedef struct
{
    uint8_t notification;
    uint8_t viewer_count;
}notification_item_t;

typedef struct
{
    QueueHandle_t handle;
    uint8_t count;
}notification_queue_t;

#define BIT_VALUE(x)    (1 << x)

typedef enum{
    COMPONENT_TYPE_NON_TASK = 0,
    COMPONENT_TYPE_TASK
}rtos_component_type_t;

#endif