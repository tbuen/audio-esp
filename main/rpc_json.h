#pragma once

#include <stdint.h>
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

uint8_t rpc_json_result_error(void *result, cJSON **json);
uint8_t rpc_json_result_get_info_connection(void *result, cJSON **json);
uint8_t rpc_json_result_get_info_about(void *result, cJSON **json);
uint8_t rpc_json_result_get_info_memory(void *result, cJSON **json);
uint8_t rpc_json_result_get_info_spiflash(void *result, cJSON **json);
uint8_t rpc_json_result_get_info_sdcard(void *result, cJSON **json);
uint8_t rpc_json_result_get_wifi_scan_result(void *result, cJSON **json);
uint8_t rpc_json_result_get_wifi_network_list(void *result, cJSON **json);
void *rpc_json_params_set_wifi_network(cJSON *params);
void *rpc_json_params_delete_wifi_network(cJSON *params);
