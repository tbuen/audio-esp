#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "message.h"

#define AUDIO_NO_ERROR          0
#define AUDIO_IO_ERROR       -100
#define AUDIO_START_ERROR    -101
#define AUDIO_BUSY_ERROR     -102

typedef struct {
    int left;
    int right;
} volume_t;

void audio_init(QueueHandle_t q);
void audio_request(const msg_audio_request_t *request);

void audio_volume(volume_t *vol, bool set);

bool audio_play(const char *filename);
