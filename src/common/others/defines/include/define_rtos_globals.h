// DEFINE RTOS GLOBALS
// SEPTEMBER 9, 2025

#ifndef _DEFINE_RTOS_GLOBALS_
#define _DEFINE_RTOS_GLOBALS_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define QUEUE_MODULE_WIFI

// Task Handles
extern TaskHandle_t handle_task_driver_wifi;
extern TaskHandle_t handle_task_driver_lcd;
extern TaskHandle_t handle_task_module_wifi;

// Queues
extern QueueHandle_t handle_queue_module_wifi;

#endif