#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void context_init(QueueHandle_t q);
void context_create(int sockfd);
void context_delete(int sockfd);
size_t context_count(void);
