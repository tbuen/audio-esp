#include <esp_log.h>
#include <freertos/idf_additions.h>
#include <nvs_flash.h>
#include <stdlib.h>

#include "message.h"
#include "filesystem.h"
#include "led.h"
#include "button.h"
#include "connection.h"
//#include "audio.h"
#include "http_server.h"
#include "wlan.h"
#include "rpc.h"

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
    //audio_init(queue);
    rpc_init();
    con_init();
    http_init();
    wlan_init();

    msg_type_int = msg_register();
    msg_type_t msg_type_button = button_msg_type();
    msg_type_t msg_type_wlan = wlan_msg_type();
    msg_type_t msg_type_con = con_msg_type();
    msg_type_t msg_type_ws_recv = http_msg_type_ws_recv();

    msg_handle_t msg_handle = msg_listen(msg_type_int | msg_type_button | msg_type_wlan | msg_type_con | msg_type_ws_recv);

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
                    break;
                case CON_DISCONNECTED:
                    if (!con_count()) {
                        led_set(LED_YELLOW, LED_OFF);
                    }
                    break;
                default:
                    break;
            }
        } else if (msg.type == msg_type_ws_recv) {
            ws_msg_t *ws_msg = msg.ptr;
            LOGI("received [%lu]: %s", ws_msg->con, ws_msg->text);
            char *response = rpc_handle_request(ws_msg->text);
            if (response) {
                http_send_ws_msg(ws_msg->con, response);
                free(response);
            }
            msg_free(&msg);
        } else {
        }
        //ESP_LOGI(TAG, "Free heap: %d - minimum: %d", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    }
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/

static void idle_check_timer_cb(TimerHandle_t timer) {
    msg_send_value(msg_type_int, MSG_IDLE_CHECK);
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
