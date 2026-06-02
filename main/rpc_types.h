#pragma once

#include <stdint.h>

#include "connection.h"
#include "filesystem.h"

/********************
***** CONSTANTS *****
********************/

#define RPC_WIFI_SCAN_RESULT_MAX_AP        10

#define RPC_ERROR_NO_ERROR                  0
#define RPC_ERROR_NOT_ALLOWED_IN_STA_MODE   1
#define RPC_ERROR_NO_SPACE_LEFT             2
#define RPC_ERROR_NOT_FOUND                 3

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
    con_mode_t mode;
} rpc_result_get_info_con_t;

typedef struct {
    char project[32];
    char version[32];
    char idf_ver[32];
    char date[16];
    char time[16];
} rpc_result_get_info_about_t;

typedef fs_web_info_t rpc_result_get_info_spiflash_t;

typedef struct {
    uint8_t error;
    uint8_t cnt;
    struct {
        char ssid[33];
        uint8_t rssi;
    } ap[RPC_WIFI_SCAN_RESULT_MAX_AP];
} rpc_result_get_wifi_scan_result_t;

typedef struct {
    uint8_t error;
    cJSON *networks;
} rpc_result_get_wifi_network_list_t;

typedef struct {
    char ssid[33];
    char key[65];
} rpc_params_set_wifi_network_t;

typedef struct {
    char ssid[33];
} rpc_params_delete_wifi_network_t;

/********************
***** FUNCTIONS *****
********************/
