#pragma once

#include <stdint.h>

/********************
***** CONSTANTS *****
********************/

#define RPC_WIFI_SCAN_RESULT_MAX_AP  10
#define RPC_WIFI_NUMBER_OF_NETWORKS   5

#define RPC_ERROR_NO_ERROR            0
#define RPC_ERROR_NO_SPACE_LEFT       1
#define RPC_ERROR_NOT_FOUND           2

/********************
***** MACROS ********
********************/

/********************
***** TYPES *********
********************/

typedef struct {
    uint8_t error;
} rpc_result_error_t;

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
    } ap[RPC_WIFI_SCAN_RESULT_MAX_AP];
} rpc_result_get_wifi_scan_result_t;

typedef struct {
    struct {
        char ssid[33];
    } network[RPC_WIFI_NUMBER_OF_NETWORKS];
} rpc_result_get_wifi_network_list_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t key[64];
} rpc_params_set_wifi_network_t;

typedef struct {
    uint8_t ssid[32];
} rpc_params_delete_wifi_network_t;

/********************
***** FUNCTIONS *****
********************/
