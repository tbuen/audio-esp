#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    WLAN_MODE_STA,
    WLAN_MODE_AP
} wlan_mode_t;

void wlan_init(QueueHandle_t q);

void wlan_set_mode(wlan_mode_t m);
void wlan_connect(const uint8_t *ssid, const uint8_t *password);
