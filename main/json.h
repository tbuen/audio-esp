#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "message.h"

void json_init(QueueHandle_t q);
void json_recv(con_t con, const char *text);
void json_send_file_list(con_t con, uint32_t rpc_id, int16_t error, audio_file_list_t *list);
