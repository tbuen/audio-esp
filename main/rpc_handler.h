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

void rpc_handler_get_info_con(void *ctx, void *params, void **result);
void rpc_handler_get_info_about(void *ctx, void *params, void **result);
void rpc_handler_get_info_spiflash(void *ctx, void *params, void **result);
void rpc_handler_get_wifi_scan_result(void *ctx, void *params, void **result);
void rpc_handler_get_wifi_network_list(void *ctx, void *params, void **result);
void rpc_handler_set_wifi_network(void *ctx, void *params, void **result);
void rpc_handler_delete_wifi_network(void *ctx, void *params, void **result);
