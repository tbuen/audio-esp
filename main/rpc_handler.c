#include <cJSON.h>
#include <esp_app_desc.h>
#include <stdlib.h>
#include <string.h>

#include "card.h"
#include "connection.h"
#include "esp_heap_caps.h"
#include "filesystem.h"
#include "freertos/idf_additions.h"
#include "multi_heap.h"
#include "wlan.h"
#include "rpc_types.h"
#include "rpc_handler.h"

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

void rpc_handler_get_info_connection(void *ctx, void *params, void **result) {
    rpc_result_get_info_connection_t *info = calloc(1, sizeof(rpc_result_get_info_connection_t));
    con_get_mode((con_id_t)ctx, &info->mode);
    *result = info;
}

void rpc_handler_get_info_about(void *ctx, void *params, void **result) {
    rpc_result_get_info_about_t *info = calloc(1, sizeof(rpc_result_get_info_about_t));
    const esp_app_desc_t *descr = esp_app_get_description();
    strncpy(info->project, descr->project_name, sizeof(info->project));
    strncpy(info->version, descr->version, sizeof(info->version));
    strncpy(info->idf_ver, descr->idf_ver, sizeof(info->idf_ver));
    strncpy(info->date, descr->date, sizeof(info->date));
    strncpy(info->time, descr->time, sizeof(info->time));
    *result = info;
}

void rpc_handler_get_info_memory(void *ctx, void *params, void **result) {
    rpc_result_get_info_memory_t *info = calloc(1, sizeof(rpc_result_get_info_memory_t));
    info->num_tasks = uxTaskGetNumberOfTasks();
    info->task_status = calloc(info->num_tasks, sizeof(TaskStatus_t));
    info->num_tasks = uxTaskGetSystemState(info->task_status, info->num_tasks, NULL);
    heap_caps_get_info(&info->heap, MALLOC_CAP_DEFAULT);
    *result = info;
}

void rpc_handler_get_info_spiflash(void *ctx, void *params, void **result) {
    rpc_result_get_info_spiflash_t *info = calloc(1, sizeof(rpc_result_get_info_spiflash_t));
    fs_web_info(info);
    *result = info;
}

void rpc_handler_get_info_sdcard(void *ctx, void *params, void **result) {
}

void rpc_handler_get_wifi_scan_result(void *ctx, void *params, void **result) {
    rpc_result_get_wifi_scan_result_t *scan_result = calloc(1, sizeof(rpc_result_get_wifi_scan_result_t));
    con_mode_t mode = CON_STA;
    con_get_mode((con_id_t)ctx, &mode);
    if (mode == CON_STA) {
        scan_result->error = RPC_ERROR_NOT_ALLOWED_IN_STA_MODE;
    } else {
        scan_result->error = RPC_ERROR_NO_ERROR;
        uint8_t cnt;
        wlan_ap_t *ap;
        if (wlan_get_scan_result(&cnt, &ap)) {
            int j = 0;
            for (int i = 0; (i < cnt) && (j < RPC_WIFI_SCAN_RESULT_MAX_AP); ++i) {
                memcpy(scan_result->ap[j].ssid, ap[i].ssid, sizeof(scan_result->ap[j].ssid));
                if (ap[i].rssi >= -67) {
                    scan_result->ap[j++].rssi = 3;
                } else if (ap[i].rssi >= -70) {
                    scan_result->ap[j++].rssi = 2;
                } else if (ap[i].rssi >= -80) {
                    scan_result->ap[j++].rssi = 1;
                }
            }
            scan_result->cnt = j;
            wlan_free_scan_result();
        }
    }
    *result = scan_result;
}

void rpc_handler_get_wifi_network_list(void *ctx, void *params, void **result) {
    rpc_result_get_wifi_network_list_t *network_list = malloc(sizeof(rpc_result_get_wifi_network_list_t));
    con_mode_t mode = CON_STA;
    con_get_mode((con_id_t)ctx, &mode);
    if (mode == CON_STA) {
        network_list->error = RPC_ERROR_NOT_ALLOWED_IN_STA_MODE;
    } else {
        network_list->error = RPC_ERROR_NO_ERROR;
        network_list->networks = cJSON_CreateArray();
        cJSON *cfg = fs_get_wifi_cfg();
        cJSON *networks = cJSON_GetObjectItemCaseSensitive(cfg, "networks");
        cJSON *network;
        cJSON_ArrayForEach(network, networks) {
            cJSON *ssid = cJSON_GetObjectItemCaseSensitive(network, "ssid");
            cJSON *nw = cJSON_CreateObject();
            cJSON_AddStringToObject(nw, "ssid", ssid->valuestring);
            cJSON_AddItemToArray(network_list->networks, nw);
        }
        fs_free_wifi_cfg(false);
    }
    *result = network_list;
}

void rpc_handler_set_wifi_network(void *ctx, void *params, void **result) {
    rpc_result_error_t *result_obj = calloc(1, sizeof(rpc_result_error_t));
    con_mode_t mode = CON_STA;
    con_get_mode((con_id_t)ctx, &mode);
    if (mode == CON_STA) {
        result_obj->error = RPC_ERROR_NOT_ALLOWED_IN_STA_MODE;
    } else {
        rpc_params_set_wifi_network_t *set = params;
        cJSON *cfg = fs_get_wifi_cfg();
        cJSON *networks = cJSON_GetObjectItemCaseSensitive(cfg, "networks");
        cJSON *network;
        bool replaced = false;
        cJSON_ArrayForEach(network, networks) {
            cJSON *ssid = cJSON_GetObjectItemCaseSensitive(network, "ssid");
            if (!strcmp(ssid->valuestring, set->ssid)) {
                cJSON *new_key = cJSON_CreateString(set->key);
                cJSON_ReplaceItemInObjectCaseSensitive(network, "key", new_key);
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            if (!networks) {
                networks = cJSON_AddArrayToObject(cfg, "networks");
            }
            network = cJSON_CreateObject();
            cJSON_AddStringToObject(network, "ssid", set->ssid);
            cJSON_AddStringToObject(network, "key", set->key);
            cJSON_AddItemToArray(networks, network);
        }
        fs_free_wifi_cfg(true);
        result_obj->error = RPC_ERROR_NO_ERROR;
    }
    free(params);
    *result = result_obj;
}

void rpc_handler_delete_wifi_network(void *ctx, void *params, void **result) {
    rpc_result_error_t *result_obj = calloc(1, sizeof(rpc_result_error_t));
    con_mode_t mode = CON_STA;
    con_get_mode((con_id_t)ctx, &mode);
    if (mode == CON_STA) {
        result_obj->error = RPC_ERROR_NOT_ALLOWED_IN_STA_MODE;
    } else {
        rpc_params_delete_wifi_network_t *del = params;
        cJSON *cfg = fs_get_wifi_cfg();
        cJSON *networks = cJSON_GetObjectItemCaseSensitive(cfg, "networks");
        cJSON *network;
        bool deleted = false;
        cJSON_ArrayForEach(network, networks) {
            cJSON *ssid = cJSON_GetObjectItemCaseSensitive(network, "ssid");
            if (!strcmp(ssid->valuestring, del->ssid)) {
                cJSON_Delete(cJSON_DetachItemViaPointer(networks, network));
                deleted = true;
                break;
            }
        }
        if (deleted) {
            fs_free_wifi_cfg(true);
            result_obj->error = RPC_ERROR_NO_ERROR;
        } else {
            fs_free_wifi_cfg(false);
            result_obj->error = RPC_ERROR_NOT_FOUND;
        }
    }
    free(params);
    *result = result_obj;
}

void rpc_handler_get_file_list(void *ctx, void *params, void **result) {
    rpc_result_get_file_list_t *result_obj = calloc(1, sizeof(rpc_result_get_file_list_t));
    rpc_params_get_file_list_t *p = params;
    char *path = CARD_MOUNT_POINT;
    if (p->path) {
        path = p->path;
    }
    if (card_get_directory_entries(path, &result_obj->entries)) {
        result_obj->path = strdup(path);
    } else {
        result_obj->error = RPC_ERROR_NOT_FOUND;
    }
    free(params);
    *result = result_obj;
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/
