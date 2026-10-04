#include <esp_log.h>
#include <fcntl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <unistd.h>

#include "audio.h"
#include "message.h"
#include "vs1053.h"

/***************************
***** CONSTANTS ************
***************************/

#define TASK_CORE                 1
#define TASK_PRIO                19
#define STACK_SIZE             4096

#define AUDIO_INT_SET_VOLUME      1
#define AUDIO_INT_GET_VOLUME      2
#define AUDIO_INT_PLAY_FILE       3
#define AUDIO_INT_STOP_PLAY       4

#define VOL_MIN     -50
#define VOL_MAX       0

/***************************
***** MACROS ***************
***************************/

#define TAG "audio"

#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)

/***************************
***** TYPES ****************
***************************/

/***************************
***** LOCAL FUNCTIONS ******
***************************/

static void audio_task(void *param);

/***************************
***** LOCAL VARIABLES ******
***************************/

static TaskHandle_t handle;
static msg_type_t   msg_type;
static msg_type_t   msg_type_int;
static msg_handle_t msg_handle;

/***************************
***** PUBLIC FUNCTIONS *****
***************************/

void audio_init(void) {
    assert(!handle);
    assert(!msg_type);
    assert(!msg_type_int);

    msg_type = msg_register();
    msg_type_int = msg_register();

    msg_handle = msg_listen(msg_type_int);

    if (xTaskCreatePinnedToCore(&audio_task, "audio-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        LOGE("could not create task");
    }
}

msg_type_t audio_msg_type(void) {
    assert(msg_type);
    return msg_type;
}

void audio_set_volume(int left, int right) {
    assert(msg_type_int);
    if (left < VOL_MIN) left = VOL_MIN;
    if (left > VOL_MAX) left = VOL_MAX;
    if (right < VOL_MIN) right = VOL_MIN;
    if (right > VOL_MAX) right = VOL_MAX;
    msg_send_with_data(msg_type_int, AUDIO_INT_SET_VOLUME, ((-2 * left) << 8) | (-2 * right));
}

void audio_get_volume(con_id_t con) {
    assert(msg_type_int);
    msg_send_with_data(msg_type_int, AUDIO_INT_GET_VOLUME, con);
}

void audio_play_file(const char *filename) {
    assert(msg_type_int);
    msg_send_with_ptr(msg_type_int, AUDIO_INT_PLAY_FILE, (void*)filename, NULL);
}

void audio_stop_play(void) {
    assert(msg_type_int);
    msg_send(msg_type_int, AUDIO_INT_STOP_PLAY);
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/

static void audio_task(void *param) {
    static uint8_t data[4096];
    uint8_t end_fill_byte = 0xFF;
    bool cancel_play = false;
    vs_info_t info;
    int fd = -1;

    if (vs_read_info(&info) == ESP_OK) {
        if (info.version == 0x0003) {
            end_fill_byte = info.endFillByte;
        }
        LOGI("ENDFILLBYTE: 0x%02X", end_fill_byte);
    }

    for (;;) {
        msg_t msg;
        if (msg_receive_timeout(msg_handle, &msg, 10)) {
            if (msg.type == msg_type_int) {
                switch (msg.value) {
                    case AUDIO_INT_SET_VOLUME:
                        vs_set_volume(msg.data);
                        break;
                    case AUDIO_INT_GET_VOLUME:
                        uint16_t vol = vs_get_volume();
                        audio_notif_t *audio_notif = calloc(1, sizeof(audio_notif_t));
                        audio_notif->con = msg.data;
                        audio_notif->volume.left = (vol >> 8) / -2;
                        audio_notif->volume.right = (vol & 0xFF) / -2;
                        msg_send_with_ptr(msg_type, AUDIO_NOTIF_VOLUME, audio_notif, NULL);
                        break;
                    case AUDIO_INT_PLAY_FILE:
                        if (fd >= 0) {
                            cancel_play = true;
                            msg_send_again(&msg);
                        } else {
                            fd = open(msg.ptr, O_RDONLY, 0);
                            msg_free(&msg);
                        }
                        break;
                    case AUDIO_INT_STOP_PLAY:
                        if (fd >= 0) {
                            cancel_play = true;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        if (fd >= 0) {
            int n = read(fd, data, sizeof(data));
            if (n > 0) {
                if (cancel_play) {
                    close(fd);
                    fd = -1;
                    LOGI("CANCEL FILE START");
                    if (n < sizeof(data)) {
                        memset(&data[n], end_fill_byte, sizeof(data) - n);
                    }
                    LOGI("status: %s", vs_get_status());
                    vs_set_cancel();
                    size_t offset = 0;
                    do {
                        vs_send_data(&data[offset], 32);
                        offset += 32;
                    } while (vs_get_cancel() && (offset < 2048));
                    if (vs_get_cancel()) {
                        // TODO SW reset if cancel is still 1
                        LOGW("cancel: 1");
                    } else {
                        LOGI("cancel: 0");
                    }
                    memset(data, end_fill_byte, sizeof(data));
                    vs_send_data(data, 2052);
                    LOGI("status: %s", vs_get_status());
                    LOGI("CANCEL FILE END");
                } else {
                    vs_send_data(data, n);
                    uint32_t pos;
                    if (vs_read_position_ms(&pos) == ESP_OK) {
                        LOGI("position: %d.%ds", pos / 1000, pos / 100 % 10);
                    }
                }
                //LOGI("status: %s", vs_get_status());
                //taskYIELD();
            } else {
                if (n == 0) {
                    LOGI("reached EOF!");
                } else {
                    LOGE("error reading file!");
                }
                close(fd);
                fd = -1;
                LOGI("END FILE START");
                LOGI("status: %s", vs_get_status());
                memset(data, end_fill_byte, sizeof(data));
                vs_send_data(data, 2052);
                vs_set_cancel();
                size_t offset = 0;
                do {
                    vs_send_data(&data[offset], 32);
                    offset += 32;
                } while (vs_get_cancel() && (offset < 2048));
                if (vs_get_cancel()) {
                    // TODO SW reset if cancel is still 1
                    LOGW("cancel: 1");
                } else {
                    LOGI("cancel: 0");
                }
                LOGI("status: %s", vs_get_status());
                LOGI("END FILE END");
            }
            cancel_play = false;
        }
    }
}
