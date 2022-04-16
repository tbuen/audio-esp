#pragma once

#include "stdbool.h"

char *json_get_info(void);
char *json_get_tracks(void);

char *json_get_volume(void);
bool json_post_volume(const char *content, char **response);

bool json_post_play(const char *content, char **response);

void json_free(void *obj);
