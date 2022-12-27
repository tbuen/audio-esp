#pragma once

typedef enum {
    BASE_BUTTON,
    BASE_WLAN,
    BASE_HTTP,
    BASE_JSON
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
    EVENT_JSON_ADD_WIFI
} event_t;

typedef struct {
    base_t base;
    event_t event;
    void *data;
} message_t;

typedef struct {
    int sockfd;
    char *text;
} http_msg_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_msg_t;
