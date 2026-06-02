#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "connection.h"
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

uint8_t rpc_json_result_get_info_con(void *result, cJSON **json) {
    rpc_result_get_info_con_t *info = result;
    *json = cJSON_CreateObject();
    cJSON_AddStringToObject(*json, "mode", info->mode == CON_STA ? "STA" : "AP");
    free(result);
    return RPC_ERROR_NO_ERROR;
}

uint8_t rpc_json_result_get_info_about(void *result, cJSON **json) {
    rpc_result_get_info_about_t *info = result;
    *json = cJSON_CreateObject();
    cJSON_AddStringToObject(*json, "project", info->project);
    cJSON_AddStringToObject(*json, "version", info->version);
    cJSON_AddStringToObject(*json, "esp-idf", info->idf_ver);
    cJSON_AddStringToObject(*json, "date", info->date);
    cJSON_AddStringToObject(*json, "time", info->time);
    free(result);
    return RPC_ERROR_NO_ERROR;
}

uint8_t rpc_json_result_get_info_spiflash(void *result, cJSON **json) {
    rpc_result_get_info_spiflash_t *info = result;
    char buffer[33] = {0};
    *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(*json, "total", info->total);
    cJSON_AddNumberToObject(*json, "free", info->free);
    cJSON *files = cJSON_CreateArray();
    for (int i = 0; i < info->num_files; ++i) {
        cJSON *file = cJSON_CreateObject();
        cJSON_AddStringToObject(file, "name", info->files[i].name);
        cJSON_AddStringToObject(file, "content-type", info->files[i].content_type);
        cJSON_AddNumberToObject(file, "size", info->files[i].size);
        for (int j = 0; j < 16; ++j) {
            sprintf(&buffer[2*j], "%02x", info->files[i].md5[j]);
        }
        cJSON_AddStringToObject(file, "md5", buffer);
        cJSON_AddItemToArray(files, file);
    }
    cJSON_AddItemToObject(*json, "files", files);
    free(result);
    return RPC_ERROR_NO_ERROR;
}

uint8_t rpc_json_result_get_wifi_scan_result(void *result, cJSON **json) {
    rpc_result_get_wifi_scan_result_t *scan_result = result;
    uint8_t error = scan_result->error;
    if (error == RPC_ERROR_NO_ERROR) {
        *json = cJSON_CreateArray();
        for (int i = 0; i < scan_result->cnt; ++i) {
            cJSON *ap = cJSON_CreateObject();
            cJSON_AddStringToObject(ap, "ssid", scan_result->ap[i].ssid);
            cJSON_AddNumberToObject(ap, "rssi", scan_result->ap[i].rssi);
            cJSON_AddItemToArray(*json, ap);
        }
    }
    free(result);
    return error;
}

uint8_t rpc_json_result_get_wifi_network_list(void *result, cJSON **json) {
    rpc_result_get_wifi_network_list_t *network_list = result;
    uint8_t error = network_list->error;
    if (error == RPC_ERROR_NO_ERROR) {
        *json = network_list->networks;
    }
    free(result);
    return error;
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
