#include "esp_log.h"
#include "esp_ota_ops.h"
#include "cJSON.h"
#include "string.h"

#include "audio.h"
#include "json.h"

#define JSON_NO_ERROR              0
#define JSON_DEFER_RESPONSE       -1
#define JSON_PARSE_ERROR      -32700
#define JSON_INVALID_REQUEST  -32600
#define JSON_METHOD_NOT_FOUND -32601
#define JSON_INVALID_PARAMS   -32602
#define JSON_INTERNAL_ERROR   -32603

typedef int16_t (*method_handler)(const cJSON *params, cJSON **result, con_t con, uint32_t rpc_id);

typedef struct {
    char *method;
    method_handler handler;
} method_entry_t;

static void json_send_result(cJSON *result, con_t con, uint32_t rpc_id);
static void json_send_error(int16_t code, con_t con, uint32_t rpc_id);
static char *json_error_message(int16_t code);
static int16_t method_get_version(const cJSON *params, cJSON **result, con_t con, uint32_t rpc_id);
static int16_t method_add_wifi(const cJSON *params, cJSON **result, con_t con, uint32_t rpc_id);
static int16_t method_get_file_list(const cJSON *params, cJSON **result, con_t con, uint32_t rpc_id);

static const char *TAG = "json";

static QueueHandle_t queue;

static method_entry_t method_table[] = {
    { "get-version",   &method_get_version },
    { "add-wifi",      &method_add_wifi },
    { "get-file-list", &method_get_file_list },
};

void json_init(QueueHandle_t q) {
    queue = q;
}

void json_recv(con_t con, const char *text) {
    ESP_LOGI(TAG, "recv: %s", text);

    int16_t error = JSON_PARSE_ERROR;
    uint32_t rpc_id = 0;

    cJSON *req = cJSON_Parse(text);
    cJSON *result = NULL;

    if (req) {
        error = JSON_INVALID_REQUEST;
        cJSON *jsonrpc = cJSON_GetObjectItemCaseSensitive(req, "jsonrpc");
        cJSON *method = cJSON_GetObjectItemCaseSensitive(req, "method");
        cJSON *params = cJSON_GetObjectItemCaseSensitive(req, "params");
        cJSON *id = cJSON_GetObjectItemCaseSensitive(req, "id");
        if (cJSON_IsNumber(id)) {
            rpc_id = id->valueint;
        }

        if (   cJSON_IsString(jsonrpc)
            && !strcmp(jsonrpc->valuestring, "2.0")
            && cJSON_IsString(method)
            && rpc_id) {
            error = JSON_METHOD_NOT_FOUND;
            for (int i = 0; i < sizeof(method_table)/sizeof(method_entry_t); ++i) {
                if (!strcmp(method->valuestring, method_table[i].method)) {
                    error = method_table[i].handler(params, &result, con, rpc_id);
                    break;
                }
            }
        }
        cJSON_Delete(req);
    }

    if (error == JSON_DEFER_RESPONSE) {
    } else if (error == JSON_NO_ERROR) {
        json_send_result(result, con, rpc_id);
    } else {
        json_send_error(error, con, rpc_id);
    }
}

void json_send_file_list(con_t con, uint32_t rpc_id, int16_t error, audio_file_list_t *list) {
    if (!error) {
        cJSON *result = cJSON_CreateObject();
        cJSON_AddBoolToObject(result, "first", list->first);
        cJSON_AddBoolToObject(result, "last", list->last);
        cJSON *files = cJSON_AddArrayToObject(result, "files");
        for (int i = 0; i < list->cnt; ++i) {
            cJSON *f = cJSON_CreateString(list->file[i].name);
            cJSON_AddItemToArray(files, f);
        }
        json_send_result(result, con, rpc_id);
    } else {
        json_send_error(error, con, rpc_id);
    }
}

static void json_send_result(cJSON *result, con_t con, uint32_t rpc_id) {
    cJSON *resp = cJSON_CreateObject();

    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    cJSON_AddItemToObject(resp, "result", result);
    cJSON_AddNumberToObject(resp, "id", rpc_id);

    char *text = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    if (text) {
        ESP_LOGI(TAG, "send: %s", text);
        msg_send_t *msg_data = calloc(1, sizeof(msg_send_t));
        msg_data->con = con;
        msg_data->text = text;
        message_t msg = { BASE_COM, EVENT_SEND, msg_data };
        xQueueSendToBack(queue, &msg, 0);
    }
}

static void json_send_error(int16_t code, con_t con, uint32_t rpc_id) {
    cJSON *resp = cJSON_CreateObject();
    cJSON *err = cJSON_CreateObject();

    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    cJSON_AddItemToObject(resp, "error", err);
    cJSON_AddNumberToObject(err, "code", code);
    cJSON_AddStringToObject(err, "message", json_error_message(code));
    if (rpc_id) {
        cJSON_AddNumberToObject(resp, "id", rpc_id);
    } else {
        cJSON_AddNullToObject(resp, "id");
    }

    char *text = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    if (text) {
        ESP_LOGI(TAG, "send: %s", text);
        msg_send_t *msg_data = calloc(1, sizeof(msg_send_t));
        msg_data->con = con;
        msg_data->text = text;
        message_t msg = { BASE_COM, EVENT_SEND, msg_data };
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
        case AUDIO_IO_ERROR:
            ret = "I/O error";
            break;
        case AUDIO_START_ERROR:
            ret = "no context available: not started or timeout";
            break;
        case AUDIO_BUSY_ERROR:
            ret = "busy, try again later";
            break;
        default:
            break;
    }
    return ret;
}

static int16_t method_get_version(const cJSON *params, cJSON **result, con_t con, uint32_t rpc_id) {
    const esp_app_desc_t *app = esp_ota_get_app_description();

    *result = cJSON_CreateObject();

    cJSON_AddStringToObject(*result, "project", app->project_name);
    cJSON_AddStringToObject(*result, "version", app->version);
    cJSON_AddStringToObject(*result, "esp-idf", app->idf_ver);

    return JSON_NO_ERROR;
}

static int16_t method_add_wifi(const cJSON *params, cJSON **result, con_t con, uint32_t rpc_id) {
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

static int16_t method_get_file_list(const cJSON *params, cJSON **result, con_t con, uint32_t rpc_id) {
    cJSON *start = cJSON_GetObjectItemCaseSensitive(params, "start");
    if (cJSON_IsBool(start)) {
        msg_audio_request_t *msg_data = calloc(1, sizeof(msg_audio_request_t));
        msg_data->con = con;
        msg_data->rpc_id = rpc_id;
        msg_data->request = AUDIO_REQ_FILE_LIST;
        msg_data->start = cJSON_IsTrue(start);
        message_t msg = { BASE_AUDIO, EVENT_AUDIO_REQUEST, msg_data };
        xQueueSendToBack(queue, &msg, 0);
        return JSON_DEFER_RESPONSE;
    } else {
        return JSON_INVALID_PARAMS;
    }
}

/*
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
