// DRIVER_LCD
// SEPTEMBER 30, 2025

#ifndef _DRIVER_LCD_
#define _DRIVER_LCD_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#define DRIVER_LCD_LVGL_TICK_PERIOD_MS      (5)

#define DRIVER_LCD_DISPLAY_PIXEL_CLK_HZ     (21 * 1000 * 1000)
#define DRIVER_LCD_DISPLAY_RESOLUTION_X     (800)
#define DRIVER_LCD_DISPLAY_RESOLUTION_Y     (480)
#define DRIVER_LCD_DISPLAY_DATA_WIDTH       (16)
#define DRIVER_LCD_DISPLAY_BITS_PER_PIXEL   (16)
#define DRIVER_LCD_DISPLAY_NUM_FBS          (2)

/*
#define DRIVER_LCD_LVGL_UPDATE(x)   \
    lvgl_port_lock(portMAX_DELAY);  \
    x;                              \
    lvgl_port_unlock();
*/
bool DRIVER_LCD_Init(void);

void DRIVER_LCD_Demo(void);

#endif