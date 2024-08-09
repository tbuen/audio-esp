#include <esp_log.h>
//#include <freertos/FreeRTOS.h>
//#include <freertos/task.h>
//#include <nvs_flash.h>
//#include "freertos/queue.h"

//#include "string.h"

#include "message.h"
//#include "nv.h"
#include "led.h"
#include "button.h"
#include "connection.h"
//#include "audio.h"
#include "http_server.h"
#include "wlan.h"
//#include "json.h"

/***************************
***** CONSTANTS ************
***************************/

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

//static void connect(void);
//static void handle_wlan_event(message_t *msg);
//static void handle_con_event(message_t *msg);
//static void handle_com_event(message_t *msg);
//static void handle_json_event(message_t *msg);
//static void handle_audio_event(message_t *msg);

/***************************
***** LOCAL VARIABLES ******
***************************/

//static wlan_mode_t wlan_mode = WLAN_MODE_STA;
//static bool connected;
//static uint8_t wifi_index;

/***************************
***** PUBLIC FUNCTIONS *****
***************************/

void app_main(void) {
    //QueueHandle_t queue;
    //message_t msg;
    //nvs_flash_init();

    msg_init();
    //nv_init();
    led_init();
    button_init();
    con_init();
    http_init();
    wlan_init();

    //queue = xQueueCreate(20, sizeof(message_t));

    //audio_init(queue);
    //wlan_init(queue);
    //http_init(queue);
    //con_init(queue);
    //json_init(queue);

    //wlan_set_mode(wlan_mode);

    msg_type_t msg_type_button = button_msg_type();
    msg_type_t msg_type_wlan = wlan_msg_type();
    msg_type_t msg_type_con = con_msg_type();
    msg_type_t msg_type_ws_recv = http_msg_type_ws_recv();

    msg_handle_t msg_handle = msg_listen(msg_type_button | msg_type_wlan | msg_type_con | msg_type_ws_recv);

    for (;;) {
        msg_t msg = msg_receive(msg_handle);
        if (msg.type == msg_type_button) {
            if (msg.value == BUTTON_PRESSED) {
                wlan_toggle_mode();
            }
        } else if (msg.type == msg_type_wlan) {
            LOGI("WLAN message received: %lu", msg.value);
            switch (msg.value) {
                case WLAN_CONNECTED:
                    led_set(LED_GREEN, LED_HIGH);
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
                    {
                        uint8_t cnt;
                        wlan_ap_t *ap;
                        if (wlan_get_scan_result(&cnt, &ap)) {
                            for (int i = 0; i < cnt; ++i) {
                                LOGI("AP %32s %d", ap[i].ssid, ap[i].rssi);
                            }
                            wlan_free_scan_result();
                        }
                    }
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
            ESP_LOGI(TAG, "received [%lu]: %s", ws_msg->con, ws_msg->text);
            msg_free(&msg);
        } else {
        }
        //ESP_LOGI(TAG, "Free heap: %d - minimum: %d", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    }
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/

/*static void connect(void) {
    if (wifi_networks[wifi_index].ssid[0] == 0) {
        wifi_index = 0;
    }
    if (wifi_networks[wifi_index].ssid[0] != 0) {
        wlan_connect(wifi_networks[wifi_index].ssid, wifi_networks[wifi_index].password);
        wifi_index = (wifi_index + 1) % NUMBER_OF_WIFI_NETWORKS;
    } else {
        ESP_LOGI(TAG, "no wifi configured");
    }
}

static void handle_wlan_event(message_t *msg) {
    switch (msg->event) {
        case EVENT_WLAN_AP_START:
            http_start();
            //led_set(LED_RED, LED_BLINK);
            break;
        case EVENT_WLAN_AP_STOP:
            //led_set(LED_RED, LED_OFF);
            http_stop();
            break;
        case EVENT_WLAN_AP_CONNECT:
            //led_set(LED_RED, LED_ON);
            break;
        case EVENT_WLAN_AP_DISCONNECT:
            //led_set(LED_RED, LED_BLINK);
            break;
        case EVENT_WLAN_STA_START:
            //led_set(LED_GREEN, LED_BLINK);
            connect();
            break;
        case EVENT_WLAN_STA_STOP:
            //led_set(LED_GREEN, LED_OFF);
            http_stop();
            break;
        case EVENT_WLAN_STA_CONNECT:
            break;
        case EVENT_WLAN_STA_DISCONNECT:
            if (connected) {
                //led_set(LED_GREEN, LED_BLINK);
                http_stop();
                connected = false;
            }
            if (wlan_mode == WLAN_MODE_STA) {
                connect();
            }
            break;
        case EVENT_WLAN_GOT_IP:
            connected = true;
            //led_set(LED_GREEN, LED_ON);
            http_start();
            break;
        case EVENT_WLAN_LOST_IP:
            if (connected) {
                //led_set(LED_GREEN, LED_BLINK);
                http_stop();
                connected = false;
            }
            break;
        default:
            break;
    }
}

static void handle_con_event(message_t *msg) {
    switch (msg->event) {
        case EVENT_CONNECTED:
            if (con_count()) {
                //led_set(LED_YELLOW, LED_ON);
            }
            break;
        case EVENT_DISCONNECTED:
            if (!con_count()) {
                //led_set(LED_YELLOW, LED_OFF);
            }
            break;
        default:
            break;
    }
}

static void handle_com_event(message_t *msg) {
    switch (msg->event) {
        case EVENT_RECV:
            if (msg->data) {
                msg_recv_t *msg_data = (msg_recv_t*)msg->data;
                json_recv(msg_data->con, msg_data->text);
                free(msg_data->text);
            }
            break;
        case EVENT_SEND:
            if (msg->data) {
                msg_send_t *msg_data = (msg_send_t*)msg->data;
                http_send(msg_data->con, msg_data->text);
                free(msg_data->text);
            }
            break;
        default:
            break;
    }
}

static void handle_json_event(message_t *msg) {
    switch (msg->event) {
        case EVENT_JSON_ADD_WIFI:
            if (msg->data) {
                wifi_msg_t *wifi_msg = (wifi_msg_t*)msg->data;
                for (int i = NUMBER_OF_WIFI_NETWORKS - 1; i > 0; --i) {
                    wifi_networks[i] = wifi_networks[i-1];
                }
                memcpy(wifi_networks[0].ssid, wifi_msg->ssid, sizeof(wifi_networks[0].ssid));
                memcpy(wifi_networks[0].password, wifi_msg->password, sizeof(wifi_networks[0].password));
                wifi_index = 0;
            }
            break;
        default:
            break;
    }
}

static void handle_audio_event(message_t *msg) {
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
