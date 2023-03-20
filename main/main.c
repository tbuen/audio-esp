#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "string.h"

#include "message.h"
#include "button.h"
#include "led.h"
#include "audio.h"
#include "wlan.h"
#include "http.h"
#include "json.h"

#define NUMBER_OF_WIFI_NETWORKS  5
#define FILESYSTEM_LABEL         "filesystem"
#define MOUNTPOINT               "/spiflash"
#define WIFI_CFG_FILE MOUNTPOINT "/wificfg.bin"

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_network_t;

static void connect(void);
static void handle_wlan_event(message_t *msg);
static void handle_http_event(message_t *msg);
static void handle_json_event(message_t *msg);
static void handle_audio_event(message_t *msg);

static const char *TAG = "main";

static wlan_mode_t wlan_mode = WLAN_MODE_STA;
static bool connected;
static wifi_network_t wifi_networks[NUMBER_OF_WIFI_NETWORKS];
static uint8_t wifi_index;

void app_main(void) {
    QueueHandle_t queue;
    message_t msg;

    queue = xQueueCreate(20, sizeof(message_t));

    esp_vfs_fat_mount_config_t fat_config = {
        .format_if_mount_failed = true,
        .max_files = 1,
        .allocation_unit_size = 0
    };
    wl_handle_t wl_handle;
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount(MOUNTPOINT, FILESYSTEM_LABEL, &fat_config, &wl_handle));
    FILE *f = fopen(WIFI_CFG_FILE, "r");
    if (f) {
        fread(&wifi_networks, sizeof(wifi_network_t), NUMBER_OF_WIFI_NETWORKS, f);
        fclose(f);
    } else {
        ESP_LOGW(TAG, "could not open %s", WIFI_CFG_FILE);
    }

    button_init(queue);
    led_init();
    audio_init(queue);
    wlan_init(queue);
    http_init(queue);
    json_init(queue);

    wlan_set_mode(wlan_mode);

    for (;;) {
        if (xQueueReceive(queue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ESP_LOGI(TAG, "received event: %02d", msg.event);
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
                case BASE_HTTP:
                    handle_http_event(&msg);
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
        }
        ESP_LOGI(TAG, "Free heap: %d - minimum: %d", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    }
}

static void connect(void) {
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
            led_set(LED_RED, LED_BLINK);
            break;
        case EVENT_WLAN_AP_STOP:
            led_set(LED_RED, LED_OFF);
            http_stop();
            break;
        case EVENT_WLAN_AP_CONNECT:
            led_set(LED_RED, LED_ON);
            break;
        case EVENT_WLAN_AP_DISCONNECT:
            led_set(LED_RED, LED_BLINK);
            break;
        case EVENT_WLAN_STA_START:
            led_set(LED_GREEN, LED_BLINK);
            connect();
            break;
        case EVENT_WLAN_STA_STOP:
            led_set(LED_GREEN, LED_OFF);
            http_stop();
            break;
        case EVENT_WLAN_STA_CONNECT:
            break;
        case EVENT_WLAN_STA_DISCONNECT:
            if (connected) {
                led_set(LED_GREEN, LED_BLINK);
                http_stop();
                connected = false;
            }
            if (wlan_mode == WLAN_MODE_STA) {
                connect();
            }
            break;
        case EVENT_WLAN_GOT_IP:
            connected = true;
            led_set(LED_GREEN, LED_ON);
            http_start();
            break;
        case EVENT_WLAN_LOST_IP:
            if (connected) {
                led_set(LED_GREEN, LED_BLINK);
                http_stop();
                connected = false;
            }
            break;
        default:
            break;
    }
}

static void handle_http_event(message_t *msg) {
    switch (msg->event) {
        case EVENT_HTTP_CONNECT:
            if (http_get_number_of_clients() > 0) {
                led_set(LED_YELLOW, LED_ON);
            }
            break;
        case EVENT_HTTP_DISCONNECT:
            if (http_get_number_of_clients() == 0) {
                led_set(LED_YELLOW, LED_OFF);
            }
            break;
        case EVENT_HTTP_RECV:
            if (msg->data) {
                msg_http_recv_t *msg_data = (msg_http_recv_t*)msg->data;
                json_recv(msg_data->com_ctx, msg_data->text);
                free(msg_data->text);
            }
            break;
        default:
            break;
    }
}

static void handle_json_event(message_t *msg) {
    switch (msg->event) {
        case EVENT_JSON_SEND:
            if (msg->data) {
                msg_json_send_t *msg_data = (msg_json_send_t*)msg->data;
                if (msg_data->com_ctx) {
                    if (http_send(msg_data->com_ctx->sockfd, msg_data->text)) {
                        if (msg_data->audio_ctx) {
                            msg_data->audio_ctx->start = false;
                            if (msg_data->audio_ctx->stop) {
                                ESP_LOGW(TAG, "send success, last chain -> free com_ctx and audio_ctx");
                                free(msg_data->com_ctx);
                                free(msg_data->audio_ctx);
                            } else {
                                ESP_LOGW(TAG, "send success, chain -> call audio request");
                                audio_request(msg_data->com_ctx, msg_data->audio_ctx);
                            }
                        } else {
                            ESP_LOGW(TAG, "send success, no chain -> free com_ctx");
                            free(msg_data->com_ctx);
                        }
                    } else {
                        ESP_LOGW(TAG, "send failed -> free com_ctx");
                        free(msg_data->com_ctx);
                        msg_data->com_ctx = NULL;
                        if (msg_data->audio_ctx) {
                            ESP_LOGW(TAG, "call audio request to clean up");
                            msg_data->audio_ctx->start = false;
                            msg_data->audio_ctx->stop = true;
                            audio_request(msg_data->com_ctx, msg_data->audio_ctx);
                        }
                    }
                }
                free(msg_data->text);
            }
            break;
        case EVENT_JSON_ADD_WIFI:
            if (msg->data) {
                wifi_msg_t *wifi_msg = (wifi_msg_t*)msg->data;
                for (int i = NUMBER_OF_WIFI_NETWORKS - 1; i > 0; --i) {
                    wifi_networks[i] = wifi_networks[i-1];
                }
                memcpy(wifi_networks[0].ssid, wifi_msg->ssid, sizeof(wifi_networks[0].ssid));
                memcpy(wifi_networks[0].password, wifi_msg->password, sizeof(wifi_networks[0].password));
                wifi_index = 0;
                FILE *f = fopen(WIFI_CFG_FILE, "w");
                if (f) {
                    fwrite(&wifi_networks, sizeof(wifi_network_t), NUMBER_OF_WIFI_NETWORKS, f);
                    fclose(f);
                } else {
                    ESP_LOGW(TAG, "could not open %s", WIFI_CFG_FILE);
                }
            }
            break;
        case EVENT_JSON_AUDIO_REQUEST:
            if (msg->data) {
                msg_json_audio_request_t *msg_data = (msg_json_audio_request_t*)msg->data;
                audio_request(msg_data->com_ctx, msg_data->audio_ctx);
            }
            break;
        default:
            break;
    }
}

static void handle_audio_event(message_t *msg) {
    switch (msg->event) {
        case EVENT_AUDIO_FILE_LIST:
            if (msg->data) {
                msg_audio_file_list_t *msg_data = (msg_audio_file_list_t*)msg->data;
                ESP_LOGW(TAG, "received file list msg - start: %d stop: %d", msg_data->audio_ctx->start, msg_data->audio_ctx->stop);
                for (int i = 0; i < msg_data->list.cnt; ++i) {
                    ESP_LOGW(TAG, "file name: %s", msg_data->list.files[i].name);
                }
                if (msg_data->com_ctx) {
                    json_send_file_list(msg_data->audio_ctx, msg_data->com_ctx, &msg_data->list);
                } else {
                    ESP_LOGW(TAG, "audio cleaned up, no com_ctx -> free audio_ctx");
                    free(msg_data->audio_ctx);
                }
                for (int i = 0; i < msg_data->list.cnt; ++i) {
                    free(msg_data->list.files[i].name);
                }
            }
            break;
        default:
            break;
    }
}
