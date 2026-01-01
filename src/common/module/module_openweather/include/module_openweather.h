// MODULE_OPENWEATHER
// JANUARY 1, 2026

#ifndef _MODULE_OPENWEATHER_
#define _MODULE_OPENWEATHER_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "util_dataqueue.h"

#define MODULE_OPENWEATHER_DATAQUEUE_MAX    (2)

bool MODULE_OPENWEATHER_Init(void);

bool MODULE_OPENWEATHER_AddCommand(util_dataqueue_item_t* dq_i);

#endif