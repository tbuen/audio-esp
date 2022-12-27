#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void json_init(QueueHandle_t q);
void json_recv(int sockfd, const char *rpc);
