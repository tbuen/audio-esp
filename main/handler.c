#include <esp_app_desc.h>
#include <string.h>

#include "handler.h"
#include "rpc.h"

/***************************
***** CONSTANTS ************
***************************/

/***************************
***** MACROS ***************
***************************/

/*#define TAG "http"

#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)*/

/***************************
***** TYPES ****************
***************************/

/*typedef struct {
    rpc_method_t method;
    const char *method_name;
} rpc_table_t;*/

/***************************
***** LOCAL FUNCTIONS ******
***************************/

static void handle_get_version(rpc_response_t *response);

//static void close_fn(httpd_handle_t hd, int sockfd);
//static esp_err_t websocket_handler(httpd_req_t *req);
//static void free_ws_msg(void *ptr);

/***************************
***** LOCAL VARIABLES ******
***************************/

/*static const rpc_table_t rpc_table[] = {
    { RPC_METHOD_GET_VERSION,          "get-version"          },
    { RPC_METHOD_GET_MEMINFO,          "get-meminfo"          },
    { RPC_METHOD_GET_WIFI_SCAN_RESULT, "get-wifi-scan-result" }
};*/

/***************************
***** PUBLIC FUNCTIONS *****
***************************/

//void rpc_init(void) {
//}

bool handle_request(const rpc_request_t *request, rpc_response_t *response) {
    bool ret = false;
    response->method = request->method;
    switch (request->method) {
        case RPC_METHOD_GET_VERSION:
            handle_get_version(response);
            ret = true;
            break;
        default:
            response->error = RPC_ERROR_NOT_IMPLEMENTED;
            ret = true;
            break;
    }
    return ret;
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/

static void handle_get_version(rpc_response_t *response) {
    const esp_app_desc_t *descr = esp_app_get_description();
    response->error = RPC_ERROR_NO_ERROR;
    strncpy(response->result.version.project, descr->project_name, sizeof(response->result.version.project));
    strncpy(response->result.version.version, descr->version, sizeof(response->result.version.version));
    strncpy(response->result.version.idf_ver, descr->idf_ver, sizeof(response->result.version.idf_ver));
    strncpy(response->result.version.date, descr->date, sizeof(response->result.version.date));
    strncpy(response->result.version.time, descr->time, sizeof(response->result.version.time));
}

/*bool rpc_parse_request(const char *text, rpc_request_t *request, char **error) {
    bool ret = false;
    json_rpc_request_t *json_rpc_request;
    if (json_rpc_parse_request(text, &json_rpc_request, error)) {
        for (uint8_t i = 0; i < sizeof(rpc_table)/sizeof(rpc_table[0]); ++i) {
            if (!strcmp(json_rpc_request->method, rpc_table[i].method_name)) {
                request->method = rpc_table[i].method;
                // TODO params
                ret = true;
                break;
            }
        }
        if (!ret) {
            json_rpc_build_error_method_not_found(json_rpc_request->id, error);
        }
        cJSON_Delete(json_rpc_request->params);
        free(json_rpc_request->method);
        free(json_rpc_request);
    }
    return ret;
}
*/
