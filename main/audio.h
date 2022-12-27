#pragma once

#include "stdbool.h"

typedef struct _file {
    char *name;
    struct _file *next;
} file_t;

typedef struct {
    int left;
    int right;
} volume_t;

void audio_init(void);

const file_t *audio_get_files(void);
void audio_free_files(file_t *file);

void audio_volume(volume_t *vol, bool set);

bool audio_play(const char *filename);
