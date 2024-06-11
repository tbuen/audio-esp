#if 0
#pragma once

#define MSG_BUTTON        0x01
#define MSG_WLAN_STATUS   0x02
#define MSG_CON           0x04
#define MSG_HTTP_WS_RECV  0x08

#define MSG_WLAN_INTERNAL 0x80

// MSG_BUTTON
#define BUTTON_PRESSED       1
// MSG_WLAN_STATUS
#define WLAN_SCAN_STARTED    1
#define WLAN_SCAN_FINISHED   2
#define WLAN_AP_STARTED      3
#define WLAN_AP_STOPPED      4
#define WLAN_AP_CONNECTED    5
#define WLAN_AP_DISCONNECTED 6
#define WLAN_CONNECTED       7
#define WLAN_DISCONNECTED    8
// MSG_CON
#define CON_CONNECTED        1
#define CON_DISCONNECTED     2

typedef uint8_t msg_handle_t;

typedef void (*msg_free_t)(void *ptr);

typedef struct {
    uint8_t type;
    union {
        uint32_t value;
        struct {
            void *ptr;
            msg_free_t free;
        };
    };
} msg_t;

void msg_init(void);

msg_handle_t msg_register(uint8_t msg_types);

void msg_send_value(uint8_t msg_type, uint32_t value);

void msg_send_ptr(uint8_t msg_type, void *ptr, msg_free_t free);

void msg_free(msg_t *msg);

msg_t msg_receive(msg_handle_t);

// TODO ab hier alt

#include "con.h"

#define FILE_LIST_SIZE  100

typedef enum {
    BASE_BUTTON,
    BASE_WLAN,
    BASE_CON,
    BASE_COM,
    BASE_JSON,
    BASE_AUDIO
} base_t;

typedef enum {
    // BASE_BUTTON
    EVENT_BUTTON_PRESSED,

    // BASE_WLAN
    EVENT_WLAN_AP_START,
    EVENT_WLAN_AP_STOP,
    EVENT_WLAN_AP_CONNECT,
    EVENT_WLAN_AP_DISCONNECT,
    EVENT_WLAN_STA_START,
    EVENT_WLAN_STA_STOP,
    EVENT_WLAN_STA_CONNECT,
    EVENT_WLAN_STA_DISCONNECT,
    EVENT_WLAN_GOT_IP,
    EVENT_WLAN_LOST_IP,

    // BASE CON
    EVENT_CONNECTED,
    EVENT_DISCONNECTED,

    // BASE_COM
    EVENT_RECV,
    EVENT_SEND,

    // TODO BASE_JSON
    EVENT_JSON_ADD_WIFI,

    // BASE_AUDIO
    EVENT_AUDIO_REQUEST,
    EVENT_AUDIO_FILE_LIST,
    EVENT_AUDIO_FILE_INFO
} event_t;

typedef struct {
    base_t base;
    event_t event;
    void *data;
} message_t;

typedef struct {
    con_t con;
    char *text;
} msg_recv_t;

typedef struct {
    con_t con;
    char *text;
} msg_send_t;

// TODO
typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_msg_t;

typedef enum {
    AUDIO_REQ_FILE_LIST,
    AUDIO_REQ_FILE_INFO
} audio_request_t;

typedef struct {
    con_t con;
    uint32_t rpc_id;
    audio_request_t request;
    union {
        bool start;
        char *filename;
    };
} msg_audio_request_t;

typedef struct {
    char *name;
} audio_file_t;

typedef struct {
    bool first;
    bool last;
    uint8_t cnt;
    audio_file_t file[FILE_LIST_SIZE];
} audio_file_list_t;

typedef struct {
    char *filename;
    char *genre;
    char *artist;
    char *album;
    char *title;
    uint16_t date;
    uint16_t track;
    uint16_t duration;
} audio_file_info_t;

typedef struct {
    con_t con;
    uint32_t rpc_id;
    int16_t error;
    union {
        audio_file_list_t list;
        audio_file_info_t info;
    };
} msg_audio_response_t;
#endif
