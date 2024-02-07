#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
//#include "freertos/queue.h"

//#include "string.h"

#include "message.h"
#include "nv.h"
#include "led.h"
#include "button.h"
//#include "audio.h"
//#include "wlan.h"
//#include "http.h"
//#include "con.h"
//#include "json.h"


//static void connect(void);
//static void handle_wlan_event(message_t *msg);
//static void handle_con_event(message_t *msg);
//static void handle_com_event(message_t *msg);
//static void handle_json_event(message_t *msg);
//static void handle_audio_event(message_t *msg);

static const char *TAG = "audio:main";

//static wlan_mode_t wlan_mode = WLAN_MODE_STA;
//static bool connected;
//static uint8_t wifi_index;

void app_main(void) {
    //QueueHandle_t queue;
    //message_t msg;

    msg_init();
    nv_init();
    led_init();
    button_init();

    //queue = xQueueCreate(20, sizeof(message_t));

    //audio_init(queue);
    //wlan_init(queue);
    //http_init(queue);
    //con_init(queue);
    //json_init(queue);

    //wlan_set_mode(wlan_mode);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        /*if (xQueueReceive(queue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ESP_LOGD(TAG, "received event: %02d", msg.event);
            switch (msg.base) {
                case BASE_BUTTON:
                    if (msg.event == EVENT_BUTTON_PRESSED) {
                        if (wlan_mode == WLAN_MODE_STA) {
                            wlan_mode = WLAN_MODE_AP;
                        } else {
                            wlan_mode = WLAN_MODE_STA;
                        }
                        http_stop();
                        wlan_set_mode(wlan_mode);
                    }
                    break;
                case BASE_WLAN:
                    handle_wlan_event(&msg);
                    break;
                case BASE_CON:
                    handle_con_event(&msg);
                    break;
                case BASE_COM:
                    handle_com_event(&msg);
                    break;
                case BASE_JSON:
                    handle_json_event(&msg);
                    break;
                case BASE_AUDIO:
                    handle_audio_event(&msg);
                    break;
                default:
                    break;
            }
            if (msg.data) {
                free(msg.data);
            }
        }*/
        //ESP_LOGI(TAG, "Free heap: %d - minimum: %d", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    }
}

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
