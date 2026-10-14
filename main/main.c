#include <esp_log.h>
#include <freertos/idf_additions.h>
#include <nvs_flash.h>
#include <stdlib.h>

#include "audio.h"
#include "button.h"
#include "card.h"
#include "connection.h"
#include "filesystem.h"
#include "http_server.h"
#include "led.h"
#include "message.h"
#include "rpc.h"
#include "rpc_types.h"
#include "vs1053.h"
#include "wlan.h"

/***************************
***** CONSTANTS ************
***************************/

#define MSG_IDLE_CHECK  1

/***************************
***** MACROS ***************
***************************/

#define TAG "main"

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

static void idle_check_timer_cb(TimerHandle_t timer);

/***************************
***** LOCAL VARIABLES ******
***************************/

static msg_type_t msg_type_int;

/***************************
***** PUBLIC FUNCTIONS *****
***************************/

void app_main(void) {
    if (nvs_flash_init() != ESP_OK) {
       ESP_ERROR_CHECK(nvs_flash_erase());
       ESP_ERROR_CHECK(nvs_flash_init());
    };

    msg_init();
    fs_init();
    led_init();
    button_init();
    vs_init();
    vs_card_init(CARD_MOUNT_POINT);
    vs_set_volume(0x3C3C);
    audio_init();
    rpc_init();
    con_init();
    http_init();
    wlan_init();

    msg_type_int = msg_register();
    msg_type_t msg_type_audio = audio_msg_type();
    msg_type_t msg_type_button = button_msg_type();
    msg_type_t msg_type_wlan = wlan_msg_type();
    msg_type_t msg_type_con = con_msg_type();
    msg_type_t msg_type_http = http_msg_type();

    msg_handle_t msg_handle = msg_listen(msg_type_int | msg_type_audio | msg_type_button | msg_type_wlan | msg_type_con | msg_type_http);

    TimerHandle_t timer = xTimerCreate("idle-check", pdMS_TO_TICKS(500), true, NULL, idle_check_timer_cb);
    xTimerStart(timer, 0);

    for (;;) {
        msg_t msg = msg_receive(msg_handle);
        if (msg.type == msg_type_int) {
            if (msg.value == MSG_IDLE_CHECK) {
                int sockfd;
                if (con_stale(&sockfd)) {
                    http_close(sockfd);
                }
            }
        } else if (msg.type == msg_type_button) {
            if (msg.value == BUTTON_PRESSED) {
                wlan_toggle_mode();
            }
        } else if (msg.type == msg_type_wlan) {
            LOGI("WLAN message received: %lu", msg.value);
            switch (msg.value) {
                case WLAN_CONNECTED:
                    led_set(LED_GREEN, LED_HIGH);
                    break;
                case WLAN_DISCONNECTED:
                    led_set(LED_GREEN, LED_OFF);
                    break;
                case WLAN_AP_STARTED:
                    led_set(LED_RED, LED_LOW);
                    break;
                case WLAN_AP_STOPPED:
                    led_set(LED_RED, LED_OFF);
                    break;
                case WLAN_AP_CONNECTED:
                    led_set(LED_RED, LED_HIGH);
                    break;
                case WLAN_AP_DISCONNECTED:
                    led_set(LED_RED, LED_LOW);
                    break;
                case WLAN_SCAN_STARTED:
                    led_set(LED_GREEN, LED_LOW);
                    break;
                case WLAN_SCAN_STOPPED:
                    led_set(LED_GREEN, LED_OFF);
                    break;
                default:
                    break;
            }
        } else if (msg.type == msg_type_con) {
            switch (msg.value) {
                case CON_CONNECTED:
                    if (con_count()) {
                        led_set(LED_YELLOW, LED_HIGH);
                    }
                    audio_send_volume_notif(msg.data);
                    audio_send_state_notif(msg.data);
                    break;
                case CON_DISCONNECTED:
                    if (!con_count()) {
                        led_set(LED_YELLOW, LED_OFF);
                    }
                    break;
                default:
                    break;
            }
        } else if (msg.type == msg_type_http) {
            switch (msg.value) {
                case HTTP_WS_RECEIVED:
                    ws_msg_t *ws_msg = msg.ptr;
                    LOGI("received [%lu]: %s", ws_msg->con, ws_msg->text);
                    char *response = rpc_handle_request(ws_msg->con, ws_msg->text);
                    if (response) {
                        LOGI("send [%lu]: %s", ws_msg->con, response);
                        http_send_ws_msg(ws_msg->con, response);
                        free(response);
                    }
                    msg_free(&msg);
                    break;
                default:
                    break;
            }
        } else if (msg.type == msg_type_audio) {
            switch (msg.value) {
                case AUDIO_NOTIF_VOLUME: {
                    audio_notif_t *ntfn = msg.ptr;
                    rpc_notif_params_audio_volume_t *params = &ntfn->volume;
                    char *notif = rpc_build_notification(RPC_NOTIF_VOLUME, params);
                    if (notif) {
                        LOGI("send notif [%lu]: %s", ntfn->con, notif);
                        http_send_ws_notif(ntfn->con, notif);
                        free(notif);
                    }
                    msg_free(&msg);
                    break;
                }
                case AUDIO_NOTIF_STATE: {
                    audio_notif_t *ntfn = msg.ptr;
                    switch (ntfn->state.mode) {
                        case AUDIO_MODE_PLAY:
                            led_set(LED_BLUE, LED_HIGH);
                            break;
                        case AUDIO_MODE_PAUSE:
                            led_set(LED_BLUE, LED_LOW);
                            break;
                        case AUDIO_MODE_STOP:
                        default:
                            led_set(LED_BLUE, LED_OFF);
                            break;
                    }
                    rpc_notif_params_audio_state_t *params = &ntfn->state;
                    char *notif = rpc_build_notification(RPC_NOTIF_AUDIO_STATE, params);
                    if (notif) {
                        LOGI("send notif [%lu]: %s", ntfn->con, notif);
                        http_send_ws_notif(ntfn->con, notif);
                        free(notif);
                    }
                    msg_free(&msg);
                    break;
                }
                default:
                    break;
            }
        } else {
        }
        //ESP_LOGI(TAG, "Free heap: %d - minimum: %d", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    }
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/

static void idle_check_timer_cb(TimerHandle_t timer) {
    msg_send(msg_type_int, MSG_IDLE_CHECK);
}

/*static void handle_audio_event(message_t *msg) {
    switch (msg->event) {
        case EVENT_AUDIO_REQUEST:
            if (msg->data) {
                msg_audio_request_t *msg_data = (msg_audio_request_t*)msg->data;
                audio_request(msg_data);
            }
            break;
        case EVENT_AUDIO_FILE_LIST:
            if (msg->data) {
                msg_audio_response_t *msg_data = (msg_audio_response_t*)msg->data;
                json_send_file_list(msg_data->con, msg_data->rpc_id, msg_data->error, &msg_data->list);
                for (int i = 0; i < msg_data->list.cnt; ++i) {
                    free(msg_data->list.file[i].name);
                }
            }
            break;
        case EVENT_AUDIO_FILE_INFO:
            if (msg->data) {
                msg_audio_response_t *msg_data = (msg_audio_response_t*)msg->data;
                json_send_file_info(msg_data->con, msg_data->rpc_id, msg_data->error, &msg_data->info);
                if (msg_data->info.filename) {
                    free(msg_data->info.filename);
                }
                if (msg_data->info.genre) {
                    free(msg_data->info.genre);
                }
                if (msg_data->info.artist) {
                    free(msg_data->info.artist);
                }
                if (msg_data->info.album) {
                    free(msg_data->info.album);
                }
                if (msg_data->info.title) {
                    free(msg_data->info.title);
                }
            }
            break;
        default:
            break;
    }
}*/
