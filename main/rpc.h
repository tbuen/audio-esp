#pragma once

#include "connection.h"

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

void rpc_init(void);
char *rpc_handle_request(con_id_t con, const char *request);
