// MODULE_LCD
// OCTOBER 4, 2025

#ifndef _MODULE_LCD_
#define _MODULE_LCD_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "driver_lcd.h"
#include "driver_api.h"

bool MODULE_LCD_Init(void);

bool MODULE_LCD_StartUI(void);
bool MODULE_LCD_Demo(void);

bool MODULE_LCD_SetIP(char* ip);
bool MODULE_LCD_SetTime(driver_api_time_info_t* ti);

#endif