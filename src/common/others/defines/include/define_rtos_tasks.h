// DEFINE RTOS TASKS
// SEPTEMBER 9, 2025

#ifndef _DEFINE_RTOS_TASKS_
#define _DEFINE_RTOS_TASKS_

#include "esp_log.h"

// Task Priority
#define TASK_PRIORITY_DRIVER_WIFI       (2)
#define TASK_PRIORITY_MODULE_WIFI       (2)
#define TASK_PRIORITY_MODULE_LCD        (2)
#define TASK_PRIORITY_LVGL              (10)

// Task Stack Depth
#define TASK_STACK_DEPTH_MODULE_WIFI    (2048)
#define TASK_STACK_DEPTH_MODULE_LCD     (2048)
#define TASK_STACK_DEPTH_LVGL           (4 * 4096)

// Task Debug Tag
#define DEBUG_TAG_DRIVER_CHIPINFO       ("D.ChipInfo")
#define DEBUG_TAG_DRIVER_APPINFO        ("D.AppInfo")
#define DEBUG_TAG_DRIVER_WIFI           ("D.Wifi")
#define DEBUG_TAG_DRIVER_LCD            ("D.Lcd")
#define DEBUG_TAG_DRIVER_SPIFFS         ("D.Spiffs")
#define DEBUG_TAG_MODULE_WIFI           ("M.Wifi")
#define DEBUG_TAG_MODULE_LCD            ("M.Lcd")
#define DEBUG_TAG_LVGL                  ("*.Lvgl")

#endif