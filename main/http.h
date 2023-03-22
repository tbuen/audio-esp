#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "con.h"

void http_init(QueueHandle_t q);
void http_start(void);
void http_stop(void);
void http_send(con_t con, char *text);
