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

/********************
***** FUNCTIONS *****
********************/

void rpc_handler_get_version(void *params, void **result);
void rpc_handler_get_info_spiflash(void *params, void **result);
void rpc_handler_get_wifi_scan_result(void *params, void **result);
void rpc_handler_get_wifi_network_list(void *params, void **result);
void rpc_handler_set_wifi_network(void *params, void **result);
void rpc_handler_delete_wifi_network(void *params, void **result);
