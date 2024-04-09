#pragma once

#include "con.h"

typedef struct {
    con_t con;
    char *text;
} http_ws_msg_t;
 
void http_start(con_mode_t mode);
void http_stop(void);
//void http_send(con_t con, char *text);
