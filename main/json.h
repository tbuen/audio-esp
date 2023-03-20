#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "message.h"

void json_init(QueueHandle_t q);
void json_recv(com_ctx_t *com_ctx, const char *text);
void json_send_file_list(audio_ctx_t *audio_ctx, com_ctx_t *com_ctx, const audio_file_list_t *list);
