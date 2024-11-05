#pragma once

#include <stdbool.h>

#include "rpc.h"

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

bool handle_request(const rpc_request_t *request, rpc_response_t *response);
