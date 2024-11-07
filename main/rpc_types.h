#pragma once

/********************
***** CONSTANTS *****
********************/

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

/********************
***** FUNCTIONS *****
********************/
