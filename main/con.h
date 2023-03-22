#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef uint32_t con_t;

void con_init(QueueHandle_t q);
void con_create(int sockfd);
void con_delete(int sockfd);
size_t con_count(void);
bool con_get_con(int sockfd, con_t *con);
bool con_get_sock(con_t con, int *sockfd);
