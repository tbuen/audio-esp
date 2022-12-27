#include "esp_log.h"
#include "esp_ota_ops.h"
#include "cJSON.h"
#include "string.h"

#include "message.h"
#include "json.h"

#define JSON_NO_ERROR              0
#define JSON_PARSE_ERROR      -32700
#define JSON_INVALID_REQUEST  -32600
#define JSON_METHOD_NOT_FOUND -32601
#define JSON_INVALID_PARAMS   -32602
#define JSON_INTERNAL_ERROR   -32603

typedef int16_t (*method_handler)(const cJSON *params, cJSON **result);

typedef struct {
    char *method;
    method_handler handler;
} method_entry_t;

static void json_send_result(int sockfd, cJSON *result, uint32_t id);
static void json_send_error(int sockfd, int16_t code, uint32_t id);
static char *json_error_message(int16_t code);
static int16_t method_get_info(const cJSON *params, cJSON **result);
static int16_t method_add_wifi(const cJSON *params, cJSON **result);

static const char *TAG = "json";

static QueueHandle_t queue;

static method_entry_t method_table[] = {
    { "get-info", &method_get_info },
    { "add-wifi", &method_add_wifi }
};

void json_init(QueueHandle_t q) {
    queue = q;
}

void json_recv(int sockfd, const char *rpc) {
    ESP_LOGI(TAG, "recv: %s", rpc);

    int16_t error = JSON_PARSE_ERROR;
    uint32_t id = 0;

    cJSON *req = cJSON_Parse(rpc);
    cJSON *result = NULL;

    if (req) {
        error = JSON_INVALID_REQUEST;
        cJSON *jsonrpc = cJSON_GetObjectItemCaseSensitive(req, "jsonrpc");
        cJSON *method = cJSON_GetObjectItemCaseSensitive(req, "method");
        cJSON *params = cJSON_GetObjectItemCaseSensitive(req, "params");
        cJSON *cid = cJSON_GetObjectItemCaseSensitive(req, "id");
        if (cJSON_IsNumber(cid)) {
            id = cid->valueint;
        }

        if (   cJSON_IsString(jsonrpc)
            && !strcmp(jsonrpc->valuestring, "2.0")
            && cJSON_IsString(method)
            && id) {
            error = JSON_METHOD_NOT_FOUND;
            for (int i = 0; i < sizeof(method_table)/sizeof(method_entry_t); ++i) {
                if (!strcmp(method->valuestring, method_table[i].method)) {
                    error = method_table[i].handler(params, &result);
                    break;
                }
            }
        }
        cJSON_Delete(req);
    }

    if (error == JSON_NO_ERROR) {
        json_send_result(sockfd, result, id);
    } else {
        json_send_error(sockfd, error, id);
    }
}

static void json_send_result(int sockfd, cJSON *result, uint32_t id) {
    cJSON *resp = cJSON_CreateObject();

    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    cJSON_AddItemToObject(resp, "result", result);
    cJSON_AddNumberToObject(resp, "id", id);

    char *rpc = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    if (rpc) {
        ESP_LOGI(TAG, "send: %s", rpc);
        http_msg_t *http_msg = malloc(sizeof(http_msg_t));
        http_msg->sockfd = sockfd;
        http_msg->text = rpc;
        message_t msg = { BASE_JSON, EVENT_JSON_SEND, http_msg };
        xQueueSendToBack(queue, &msg, 0);
    }
}

static void json_send_error(int sockfd, int16_t code, uint32_t id) {
    cJSON *resp = cJSON_CreateObject();
    cJSON *err = cJSON_CreateObject();

    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    cJSON_AddItemToObject(resp, "error", err);
    cJSON_AddNumberToObject(err, "code", code);
    cJSON_AddStringToObject(err, "message", json_error_message(code));
    if (id) {
        cJSON_AddNumberToObject(resp, "id", id);
    } else {
        cJSON_AddNullToObject(resp, "id");
    }

    char *rpc = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    if (rpc) {
        ESP_LOGI(TAG, "send: %s", rpc);
        http_msg_t *http_msg = malloc(sizeof(http_msg_t));
        http_msg->sockfd = sockfd;
        http_msg->text = rpc;
        message_t msg = { BASE_JSON, EVENT_JSON_SEND, http_msg };
        xQueueSendToBack(queue, &msg, 0);
    }
}

static char *json_error_message(int16_t code) {
    char *ret = "";
    switch (code) {
        case JSON_PARSE_ERROR:
            ret = "parse error";
            break;
        case JSON_INVALID_REQUEST:
            ret = "invalid request";
            break;
        case JSON_METHOD_NOT_FOUND:
            ret = "method not found";
            break;
        case JSON_INVALID_PARAMS:
            ret = "invalid params";
            break;
        case JSON_INTERNAL_ERROR:
            ret = "internal error";
            break;
        default:
            break;
    }
    return ret;
}

static int16_t method_get_info(const cJSON *params, cJSON **result) {
    const esp_app_desc_t *app = esp_ota_get_app_description();

    *result = cJSON_CreateObject();

    cJSON_AddStringToObject(*result, "project", app->project_name);
    cJSON_AddStringToObject(*result, "version", app->version);
    cJSON_AddStringToObject(*result, "esp-idf", app->idf_ver);

    return JSON_NO_ERROR;
}

static int16_t method_add_wifi(const cJSON *params, cJSON **result) {
    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(params, "ssid");
    cJSON *password = cJSON_GetObjectItemCaseSensitive(params, "password");

    if (   cJSON_IsString(ssid)
        && (strlen(ssid->valuestring) > 0)
        && (strlen(ssid->valuestring) <= 32)
        && cJSON_IsString(password)
        && (strlen(password->valuestring) > 0)
        && (strlen(password->valuestring) <= 64)) {
        *result = cJSON_CreateObject();
        wifi_msg_t *wifi_msg = calloc(1, sizeof(wifi_msg_t));
        memcpy(wifi_msg->ssid, ssid->valuestring, strlen(ssid->valuestring));
        memcpy(wifi_msg->password, password->valuestring, strlen(password->valuestring));
        message_t msg = { BASE_JSON, EVENT_JSON_ADD_WIFI, wifi_msg };
        xQueueSendToBack(queue, &msg, 0);
        return JSON_NO_ERROR;
    } else {
        return JSON_INVALID_PARAMS;
    }
}

/*
char *json_get_files(void) {
    const file_t *file = audio_get_files();
    char *string;

    cJSON *resp = cJSON_CreateArray();

    while (file) {
        cJSON *f = cJSON_CreateString(file->name);
        cJSON_AddItemToArray(resp, f);
        file = file->next;
    }

    string = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);

    audio_free_files(file);

    return string;
}

char *json_get_volume(void) {
    volume_t vol;
    audio_volume(&vol, false);
    char *string;

    cJSON *resp = cJSON_CreateObject();

    cJSON_AddNumberToObject(resp, "left", vol.left);
    cJSON_AddNumberToObject(resp, "right", vol.right);

    string = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);

    return string;
}

bool json_post_volume(const char *content, char **response) {
    bool valid = false;

    cJSON *req = cJSON_Parse(content);

    if (req) {
        cJSON *left = cJSON_GetObjectItemCaseSensitive(req, "left");
        cJSON *right = cJSON_GetObjectItemCaseSensitive(req, "right");
        if (cJSON_IsNumber(left) && cJSON_IsNumber(right)) {
            valid = true;

            volume_t vol = {
                .left = left->valueint,
                .right = right->valueint,
            };
            audio_volume(&vol, true);

            cJSON *resp = cJSON_CreateObject();

            cJSON_AddNumberToObject(resp, "left", vol.left);
            cJSON_AddNumberToObject(resp, "right", vol.right);

            *response = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
        }
        cJSON_Delete(req);
    }

    return valid;
}

bool json_post_play(const char *content, char **response) {
    bool valid = false;

    cJSON *req = cJSON_Parse(content);

    if (req) {
        cJSON *filename = cJSON_GetObjectItemCaseSensitive(req, "filename");
        if (cJSON_IsString(filename) && filename->valuestring) {
            valid = true;

            bool status = audio_play(filename->valuestring);

            cJSON *resp = cJSON_CreateObject();

            cJSON_AddBoolToObject(resp, "status", status);

            *response = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
        }
        cJSON_Delete(req);
    }

    return valid;
}*/
