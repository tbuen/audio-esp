#pragma once

#include "stdbool.h"

char *json_get_info(void);
char *json_get_tracks(void);

bool json_post_play(const char *content, char **response);

void json_free(void *obj);
