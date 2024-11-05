#pragma once

#include <stdbool.h>

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

typedef enum {
    RPC_ERROR_NO_ERROR,
    RPC_ERROR_NOT_IMPLEMENTED
} rpc_error_t;

typedef struct {
    rpc_method_t method;
} rpc_request_t;

typedef struct {
    char project[32];
    char version[32];
    char idf_ver[32];
    char date[16];
    char time[16];
} rpc_version_t;

typedef union {
    rpc_version_t version;
} rpc_result_t;

typedef struct {
    rpc_method_t method;
    rpc_error_t error;
    rpc_result_t result;
} rpc_response_t;

/********************
***** FUNCTIONS *****
********************/

void rpc_init(void);
bool rpc_parse_request(const char *text, rpc_request_t *request, char **error);
void rpc_build_response(const rpc_response_t *response, char **resp);
