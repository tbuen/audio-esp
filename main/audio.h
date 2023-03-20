#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "message.h"

typedef struct {
    int left;
    int right;
} volume_t;

void audio_init(QueueHandle_t q);
void audio_request(com_ctx_t *com_ctx, audio_ctx_t *audio_ctx);

void audio_volume(volume_t *vol, bool set);

bool audio_play(const char *filename);
