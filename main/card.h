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
    char *cover;
    entry_t *dirs;
    entry_t *tracks;
} dir_entries_t;

typedef struct {
    char *genre;
    char *artist;
    char *album;
    char *title;
    uint16_t date;
    uint16_t track;
    uint16_t duration;
} track_info_t;

/********************
***** FUNCTIONS *****
********************/

bool card_get_directory_entries(const char *path, dir_entries_t *entries);
uint8_t card_get_track_info(const char *filename, track_info_t *info);
