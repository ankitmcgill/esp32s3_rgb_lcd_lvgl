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
#define MODULE_API_DATAQUEUE_MAX            (3)
#define MODULE_API_NOTIFICATION_TARGET_MAX  (1)


bool MODULE_API_Init(void);

bool MODULE_API_AddNotificationTarget(util_dataqueue_t* dq);

#endif