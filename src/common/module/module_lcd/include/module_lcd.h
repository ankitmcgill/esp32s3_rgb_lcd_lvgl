// MODULE_LCD
// OCTOBER 4, 2025

#ifndef _MODULE_LCD_
#define _MODULE_LCD_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "driver_lcd.h"

bool MODULE_LCD_Init(void);

void MODULE_LCD_SetUIFunction(void (*ptr)(void));
bool MODULE_LCD_StartUI(void);
bool MODULE_LCD_Demo(void);

#endif