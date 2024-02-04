#include "esp_log.h"
#include "esp_wifi.h"
#include "mdns.h"
#include "string.h"

#include "message.h"
#include "wlan.h"

#define TASK_CORE     1
#define TASK_PRIO     1
#define STACK_SIZE 4096

typedef enum {
    WLAN_REQ_STA,
    WLAN_REQ_AP,
    WLAN_REQ_CONNECT
} req_t;

typedef struct {
    req_t req;
    void *data;
} request_t;

static void wlan_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wlan_sta(bool start);
static void wlan_ap(bool start);
static void wlan_con(const uint8_t *ssid, const uint8_t *password);
static void wlan_task(void *param);

static const char *TAG = "wlan";

static TaskHandle_t handle;
static QueueHandle_t queue;
static QueueHandle_t request_queue;
static msg_handle_t msg_handle;

void wlan_init(QueueHandle_t q) {
    if (handle) return;

    msg_handle = msg_register(MSG_BUTTON);

    queue = q;

    ESP_LOGI(TAG, "Task init: %s", pcTaskGetName(NULL));
    request_queue = xQueueCreate(5, sizeof(request_t));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *intf_ap = esp_netif_create_default_wifi_ap();
    esp_netif_t *intf_sta = esp_netif_create_default_wifi_sta();

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(intf_ap, &ip_info);
    ESP_LOGI(TAG, "AP IP address: " IPSTR, IP2STR(&ip_info.ip));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_country_code("DEI", true));
    char country[10] = {0};
    ESP_ERROR_CHECK(esp_wifi_get_country_code(country));
    ESP_LOGI(TAG, "Country Code: %s", country);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wlan_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wlan_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_netif_set_hostname(intf_ap, "esp32"));
    ESP_ERROR_CHECK(esp_netif_set_hostname(intf_sta, "esp32"));

    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("esp32"));
    ESP_ERROR_CHECK(mdns_service_add("audio-esp", "_audio-jsonrpc-ws", "_tcp", 80, NULL, 0));

    if (xTaskCreatePinnedToCore(&wlan_task, "wlan-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

void wlan_set_mode(wlan_mode_t m) {
    if (m == WLAN_MODE_STA) {
        request_t request = { WLAN_REQ_STA, NULL };
        xQueueSendToBack(request_queue, &request, 0);
    } else if (m == WLAN_MODE_AP) {
        request_t request = { WLAN_REQ_AP, NULL };
        xQueueSendToBack(request_queue, &request, 0);
    }
}

void wlan_connect(const uint8_t *ssid, const uint8_t *password) {
    wifi_msg_t *wifi_msg = calloc(1, sizeof(wifi_msg_t));
    memcpy(wifi_msg->ssid, ssid, sizeof(wifi_msg->ssid));
    memcpy(wifi_msg->password, password, sizeof(wifi_msg->password));
    request_t request = { WLAN_REQ_CONNECT, wifi_msg };
    xQueueSendToBack(request_queue, &request, 0);
}

static void wlan_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Task event: %s", pcTaskGetName(NULL));
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                {
                    message_t msg = { BASE_WLAN, EVENT_WLAN_STA_START, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            case WIFI_EVENT_STA_STOP:
                {
                    message_t msg = { BASE_WLAN, EVENT_WLAN_STA_STOP, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            case WIFI_EVENT_SCAN_DONE:
                {
                    uint16_t number = 10;
                    wifi_ap_record_t records[10];
                    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&number));
                    ESP_LOGI(TAG, "found %d APs", number);
                    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, records));
                    for (int i = 0; i < number; ++i) {
                        ESP_LOGI(TAG, "FOUND AP: SSID %s CH %d RSSI %d WPS %d AUTH %d, CC %s", records[i].ssid, records[i].primary, records[i].rssi, records[i].wps, records[i].authmode, records[i].country.cc);
                    }
                }
                break;
            case WIFI_EVENT_STA_CONNECTED:
                {
                    message_t msg = { BASE_WLAN, EVENT_WLAN_STA_CONNECT, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                {
                    ESP_LOGI(TAG, "STA disconnected, reason: %d", ((wifi_event_sta_disconnected_t*)event_data)->reason);
                    message_t msg = { BASE_WLAN, EVENT_WLAN_STA_DISCONNECT, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            case WIFI_EVENT_AP_START:
                {
                    message_t msg = { BASE_WLAN, EVENT_WLAN_AP_START, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            case WIFI_EVENT_AP_STOP:
                {
                    message_t msg = { BASE_WLAN, EVENT_WLAN_AP_STOP, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                {
                    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
                    ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
                    message_t msg = { BASE_WLAN, EVENT_WLAN_AP_CONNECT, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                {
                    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
                    ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
                    message_t msg = { BASE_WLAN, EVENT_WLAN_AP_DISCONNECT, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            default:
                break;
        }
    }
    if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                {
                    ip_event_got_ip_t *event = (ip_event_got_ip_t*)event_data;
                    ESP_LOGI(TAG, "GOT IP: " IPSTR, IP2STR(&event->ip_info.ip));
                    message_t msg = { BASE_WLAN, EVENT_WLAN_GOT_IP, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            case IP_EVENT_STA_LOST_IP:
                {
                    ESP_LOGI(TAG, "LOST IP");
                    message_t msg = { BASE_WLAN, EVENT_WLAN_LOST_IP, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
                break;
            default:
                break;
        }
    }
}

static void wlan_sta(bool start) {
    if (start) {
        wifi_scan_config_t config = {
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_scan_start(&config, false));
    } else {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_stop());
    }
}

static void wlan_ap(bool start) {
    if (start) {
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = "esp32-audio",
                .channel = 6,
                .password = "audio-esp",
                .max_connection = 1,
                .authmode = WIFI_AUTH_WPA2_PSK
            },
        };

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
    } else {
        ESP_ERROR_CHECK(esp_wifi_deauth_sta(0));
        ESP_ERROR_CHECK(esp_wifi_stop());
    }
}

static void wlan_con(const uint8_t *ssid, const uint8_t *password) {
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                //.capable = true,
                //.required = false
                .required = true
            },
        },
    };
    memcpy(&wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    memcpy(&wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void wlan_task(void *param) {
    wifi_mode_t mode = WIFI_MODE_NULL;
    ESP_LOGI(TAG, "Task wlan: %s", pcTaskGetName(NULL));
    for (;;) {
        request_t request;
        if (xQueueReceive(request_queue, &request, portMAX_DELAY) == pdTRUE) {
            switch (request.req) {
                case WLAN_REQ_STA:
                    if (mode == WIFI_MODE_AP) {
                        wlan_ap(false);
                    }
                    wlan_sta(true);
                    mode = WIFI_MODE_STA;
                    break;
                case WLAN_REQ_AP:
                    if (mode == WIFI_MODE_STA) {
                        wlan_sta(false);
                    }
                    wlan_ap(true);
                    mode = WIFI_MODE_AP;
                    break;
#if 0
                case WLAN_REQ_CONNECT:
                    if (   (mode == WIFI_MODE_STA)
                        && request.data) {
                        wifi_msg_t *wifi_msg = (wifi_msg_t*)request.data;
                        wlan_con(wifi_msg->ssid, wifi_msg->password);
                        free(request.data);
                    }
                    break;
#endif
                default:
                    break;
            }
        }
    }
}
