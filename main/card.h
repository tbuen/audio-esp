#pragma once

#include <stdint.h>

/********************
***** CONSTANTS *****
********************/

#define CARD_MOUNT_POINT "/sdcard"

/********************
***** MACROS ********
********************/

/********************
***** TYPES *********
********************/

typedef struct entry {
    char *name;
    struct entry *next;
} entry_t;

typedef struct {
    entry_t *dirs;
    entry_t *files;
} dir_entries_t;

/********************
***** FUNCTIONS *****
********************/

bool card_get_directory_entries(const char *path, dir_entries_t *entries);
