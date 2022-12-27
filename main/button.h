#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void button_init(QueueHandle_t q);
