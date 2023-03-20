#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void   http_init(QueueHandle_t q);
void   http_start(void);
void   http_stop(void);
bool   http_send(int sockfd, char *text);
