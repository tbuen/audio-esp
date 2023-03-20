#pragma once

#define FILE_LIST_SIZE  100

typedef enum {
    BASE_BUTTON,
    BASE_WLAN,
    BASE_HTTP,
    BASE_JSON,
    BASE_AUDIO
} base_t;

typedef enum {
    EVENT_BUTTON_PRESSED,

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

    EVENT_HTTP_CONNECT,
    EVENT_HTTP_DISCONNECT,
    EVENT_HTTP_RECV,

    EVENT_JSON_SEND,
    EVENT_JSON_ADD_WIFI,
    EVENT_JSON_AUDIO_REQUEST,

    EVENT_AUDIO_FILE_LIST
} event_t;

typedef struct {
    base_t base;
    event_t event;
    void *data;
} message_t;

typedef struct {
    int sockfd;
    uint32_t id;
    bool error;
} com_ctx_t;

typedef enum {
    AUDIO_REQ_FILE_LIST
} audio_request_t;

typedef struct {
    audio_request_t request;
    bool start;
    bool stop;
    void *data;
} audio_ctx_t;

typedef struct {
    com_ctx_t *com_ctx;
    char *text;
} msg_http_recv_t;

typedef struct {
    com_ctx_t *com_ctx;
    audio_ctx_t *audio_ctx;
    char *text;
} msg_json_send_t;

// TODO
typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_msg_t;

typedef struct {
    audio_ctx_t *audio_ctx;
    com_ctx_t *com_ctx;
} msg_json_audio_request_t;

typedef struct {
    char *name;
} audio_file_t;

typedef struct {
    uint8_t cnt;
    audio_file_t files[FILE_LIST_SIZE];
} audio_file_list_t;

typedef struct {
    audio_ctx_t *audio_ctx;
    com_ctx_t *com_ctx;
    audio_file_list_t list;
} msg_audio_file_list_t;
