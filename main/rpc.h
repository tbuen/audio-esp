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

typedef enum {
    RPC_METHOD_GET_VERSION,
    RPC_METHOD_GET_MEMINFO,
    RPC_METHOD_GET_WIFI_SCAN_RESULT
} rpc_method_t;

typedef struct {
    rpc_method_t method;
} rpc_request_t;

/********************
***** FUNCTIONS *****
********************/

void rpc_init(void);
bool rpc_parse_request(const char *text, rpc_request_t *request, char **error);
