#pragma once

#include <cJSON.h>

/********************
***** CONSTANTS *****
********************/

/********************
***** MACROS ********
********************/

/********************
***** TYPES *********
********************/

/********************
***** FUNCTIONS *****
********************/

cJSON *rpc_json_result_get_version(void *result);
cJSON *rpc_json_result_get_wifi_scan_result(void *result);
