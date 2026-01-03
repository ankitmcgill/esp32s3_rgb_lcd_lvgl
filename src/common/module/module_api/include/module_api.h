// MODULE_API
// JANUARY 1, 2026

#ifndef _MODULE_API_
#define _MODULE_API_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "util_dataqueue.h"

#define MODULE_API_EXECUTE_PERIOD_S     (30)
#define MODULE_API_DATAQUEUE_MAX        (2)

bool MODULE_API_Init(void);

#endif