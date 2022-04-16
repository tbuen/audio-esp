#pragma once

#include "stdbool.h"

typedef struct _track {
    char *filename;
    struct _track *next;
} audio_track;

typedef struct {
    int left;
    int right;
} volume_t;

void audio_init(void);

const audio_track *audio_get_tracks(void);
void audio_release_tracks(void);

void audio_volume(volume_t *vol, bool set);

bool audio_play(const char *filename);
