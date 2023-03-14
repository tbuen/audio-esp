#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    int left;
    int right;
} volume_t;

void audio_init(QueueHandle_t q);
void audio_get_file_list(void *ctx);

void audio_volume(volume_t *vol, bool set);

bool audio_play(const char *filename);
