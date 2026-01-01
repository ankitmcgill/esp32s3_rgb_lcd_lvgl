// DRIVER_LCD
// SEPTEMBER 30, 2025

#ifndef _DRIVER_LCD_
#define _DRIVER_LCD_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "util_dataqueue.h"

#define DRIVER_LCD_LVGL_TICK_PERIOD_MS      (2)
#define DRIVER_LCD_LVGL_TASK_PERIOD_MS      (10)

#define DRIVER_LCD_DISPLAY_HRES             (800)
#define DRIVER_LCD_DISPLAY_VRES             (480)
#define DRIVER_LCD_UI_HRES                  (DRIVER_LCD_DISPLAY_HRES)
#define DRIVER_LCD_UI_VRES                  (DRIVER_LCD_DISPLAY_VRES)

#define DRIVER_LCD_DATAQUEUE_MAX            (2)

typedef enum {
    DRIVER_LCD_COMMAND_DEMO = 0,
    DRIVER_LCD_COMMAND_LOAD_UI
}driver_lcd_command_type_t;

bool DRIVER_LCD_Init(void);

bool DRIVER_LCD_AddCommand(util_dataqueue_item_t* dq_i);

#endif