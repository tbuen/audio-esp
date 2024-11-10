#pragma once

#include <stdint.h>

/********************
***** CONSTANTS *****
********************/

#define RPC_WLAN_SCAN_RESULT_MAX_AP  10

/********************
***** MACROS ********
********************/

/********************
***** TYPES *********
********************/

typedef struct {
    char project[32];
    char version[32];
    char idf_ver[32];
    char date[16];
    char time[16];
} rpc_result_get_version_t;

typedef struct {
    uint8_t cnt;
    struct {
        char ssid[33];
        uint8_t rssi;
    } ap[RPC_WLAN_SCAN_RESULT_MAX_AP];
} rpc_result_get_wlan_scan_result_t;

/********************
***** FUNCTIONS *****
********************/
