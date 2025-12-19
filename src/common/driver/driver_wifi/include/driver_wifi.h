// DRIVER_WIFI
// SEPTEMBER 6, 2025

#ifndef _DRIVER_WIFI_
#define _DRIVER_WIFI_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "esp_wifi.h"

#define DRIVER_WIFI_LEN_SSID_MAX                (32)
#define DRIVER_WIFI_LED_PWD_MAX                 (64)

#define DRIVER_WIFI_SCAN_RESULTS_COUNT_MAX      (10)

#define DRIVER_WIFI_MESSAGE_QUEUE_MAX           (2)
#define DRIVER_WIFI_NOTIFICATION_QUEUE_MAX      (2)

typedef enum {
    DRIVER_WIFI_MESSAGE_SCAN = 0,
    DRIVER_WIFI_MESSAGE_CONNECT,
    DRIVER_WIFI_MESSAGE_DISCONNECT
}driver_wifi_message_type_t;

typedef enum{
    DRIVER_WIFI_NOTIFICATION_SCAN_DONE = 0,
    DRIVER_WIFI_NOTIFICATION_CONNECTED,
    DRIVER_WIFI_NOTIFICATION_GOT_IP,
    DRIVER_WIFI_NOTIFICATION_LOST_IP,
    DRIVER_WIFI_NOTIFICATION_DISCONNECTED
}driver_wifi_notification_type_t;

bool DRIVER_WIFI_Init(void);

bool DRIVER_WIFI_SendMessage(driver_wifi_message_type_t message, TickType_t wait);
bool DRIVER_WIFI_CheckNotification(driver_wifi_notification_type_t* notification, TickType_t wait);

bool DRIVER_WIFI_CheckSavedWifiCredentials(void);
void DRIVER_WIFI_Connect(char* ssid, char* password);

#endif