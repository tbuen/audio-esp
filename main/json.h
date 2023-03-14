#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void json_init(QueueHandle_t q);
void json_recv(int sockfd, const char *rpc);
void json_send_file_list(const void *ctx, const audio_file_list_t *list);
