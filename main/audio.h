#pragma once

#include <stdint.h>

#include "connection.h"
#include "message.h"

/********************
***** CONSTANTS *****
********************/

#define AUDIO_NOTIF_VOLUME  1
#define AUDIO_NOTIF_STATE   2

/********************
***** MACROS ********
********************/

/********************
***** TYPES *********
********************/

typedef enum {
    AUDIO_MODE_STOP,
    AUDIO_MODE_PAUSE,
    AUDIO_MODE_PLAY
} audio_mode_t;

typedef struct {
    int8_t left;
    int8_t right;
} audio_volume_t;

typedef struct {
    audio_mode_t mode;
    char *file;
    uint16_t pos;
} audio_state_t;

typedef struct {
    con_id_t con;
    union {
        audio_volume_t volume;
        audio_state_t  state;
    };
} audio_notif_t;

/********************
***** FUNCTIONS *****
********************/

void        audio_init(void);
msg_type_t  audio_msg_type(void);
void        audio_set_volume(int left, int right);
void        audio_play_file(const char *filename);
void        audio_stop_play(void);
void        audio_send_volume_notif(con_id_t con);
void        audio_send_state_notif(con_id_t con);
