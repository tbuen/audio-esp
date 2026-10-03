#pragma once

#include <stdint.h>

#include "connection.h"
#include "message.h"

/********************
***** CONSTANTS *****
********************/

#define AUDIO_VOLUME    1

/********************
***** MACROS ********
********************/

/********************
***** TYPES *********
********************/

typedef struct {
    con_id_t con;
    uint8_t type;
    union {
        struct {
            int8_t left;
            int8_t right;
        } volume;
    };
} audio_notif_t;

//#define AUDIO_NO_ERROR                0
//#define AUDIO_IO_ERROR             -100
//#define AUDIO_START_ERROR          -101
//#define AUDIO_BUSY_ERROR           -102
//#define AUDIO_FILE_NOT_FOUND_ERROR -103
//#define AUDIO_FILE_TYPE_ERROR      -104

/********************
***** FUNCTIONS *****
********************/

void        audio_init(void);
msg_type_t  audio_msg_type(void);
void        audio_set_volume(int left, int right);
void        audio_get_volume(con_id_t con);

//void audio_request(const msg_audio_request_t *request);
//bool audio_play(const char *filename);
