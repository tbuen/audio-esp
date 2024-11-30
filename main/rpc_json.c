#include <stdlib.h>
#include <string.h>

#include "rpc_types.h"
#include "rpc_json.h"

/***************************
***** CONSTANTS ************
***************************/

/***************************
***** MACROS ***************
***************************/

/***************************
***** TYPES ****************
***************************/

/***************************
***** LOCAL FUNCTIONS ******
***************************/

/***************************
***** LOCAL VARIABLES ******
***************************/

/***************************
***** PUBLIC FUNCTIONS *****
***************************/

uint8_t rpc_json_result_error(void *result, cJSON **json) {
    rpc_result_error_t *result_obj = result;
    uint8_t error = result_obj->error;
    if (error == RPC_ERROR_NO_ERROR) {
        *json = cJSON_CreateNull();
    }
    free(result);
    return error;
}

uint8_t rpc_json_result_get_version(void *result, cJSON **json) {
    rpc_result_get_version_t *version = result;
    *json = cJSON_CreateObject();
    cJSON_AddStringToObject(*json, "project", version->project);
    cJSON_AddStringToObject(*json, "version", version->version);
    cJSON_AddStringToObject(*json, "esp-idf", version->idf_ver);
    cJSON_AddStringToObject(*json, "date", version->date);
    cJSON_AddStringToObject(*json, "time", version->time);
    free(result);
    return RPC_ERROR_NO_ERROR;
}

uint8_t rpc_json_result_get_info_spiflash(void *result, cJSON **json) {
    rpc_result_get_info_spiflash_t *info = result;
    *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(*json, "total", info->total);
    cJSON_AddNumberToObject(*json, "free", info->free);
    cJSON *files = cJSON_CreateArray();
    for (int i = 0; i < info->num_files; ++i) {
        cJSON *file = cJSON_CreateObject();
        cJSON_AddStringToObject(file, "name", info->files[i].name);
        cJSON_AddStringToObject(file, "content-type", info->files[i].content_type);
        cJSON_AddNumberToObject(file, "size", info->files[i].size);
        cJSON_AddItemToArray(files, file);
    }
    cJSON_AddItemToObject(*json, "files", files);
    free(result);
    return RPC_ERROR_NO_ERROR;
}

uint8_t rpc_json_result_get_wifi_scan_result(void *result, cJSON **json) {
    rpc_result_get_wifi_scan_result_t *scan_result = result;
    *json = cJSON_CreateArray();
    for (int i = 0; i < scan_result->cnt; ++i) {
        cJSON *ap = cJSON_CreateObject();
        cJSON_AddStringToObject(ap, "ssid", scan_result->ap[i].ssid);
        cJSON_AddNumberToObject(ap, "rssi", scan_result->ap[i].rssi);
        cJSON_AddItemToArray(*json, ap);
    }
    free(result);
    return RPC_ERROR_NO_ERROR;
}

uint8_t rpc_json_result_get_wifi_network_list(void *result, cJSON **json) {
    rpc_result_get_wifi_network_list_t *network_list = result;
    *json = cJSON_CreateArray();
    for (int i = 0; i < RPC_WIFI_NUMBER_OF_NETWORKS; ++i) {
        if (network_list->network[i].ssid[0]) {
            cJSON *network = cJSON_CreateObject();
            cJSON_AddStringToObject(network, "ssid", network_list->network[i].ssid);
            cJSON_AddItemToArray(*json, network);
        }
    }
    free(result);
    return RPC_ERROR_NO_ERROR;
}

void *rpc_json_params_set_wifi_network(cJSON *params) {
    rpc_params_set_wifi_network_t *obj = NULL;
    if (cJSON_IsObject(params)) {
        cJSON *ssid = cJSON_GetObjectItemCaseSensitive(params, "ssid");
        cJSON *key = cJSON_GetObjectItemCaseSensitive(params, "key");
        if (   cJSON_IsString(ssid)
            && (strlen(ssid->valuestring) > 0)
            && (strlen(ssid->valuestring) <= 32)
            && cJSON_IsString(key)
            && (strlen(key->valuestring) > 0)
            && (strlen(key->valuestring) <= 64)) {
            obj = calloc(1, sizeof(rpc_params_set_wifi_network_t));
            memcpy(obj->ssid, ssid->valuestring, strlen(ssid->valuestring));
            memcpy(obj->key, key->valuestring, strlen(key->valuestring));
        }
    }
    return obj;
}

void *rpc_json_params_delete_wifi_network(cJSON *params) {
    rpc_params_delete_wifi_network_t *obj = NULL;
    if (cJSON_IsObject(params)) {
        cJSON *ssid = cJSON_GetObjectItemCaseSensitive(params, "ssid");
        if (   cJSON_IsString(ssid)
            && (strlen(ssid->valuestring) > 0)
            && (strlen(ssid->valuestring) <= 32)) {
            obj = calloc(1, sizeof(rpc_params_delete_wifi_network_t));
            memcpy(obj->ssid, ssid->valuestring, strlen(ssid->valuestring));
        }
    }
    return obj;
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/
