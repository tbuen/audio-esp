#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
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
        *json = cJSON_CreateObject();
    }
    free(result);
    return error;
}

uint8_t rpc_json_result_get_info_connection(void *result, cJSON **json) {
    rpc_result_get_info_connection_t *info = result;
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

uint8_t rpc_json_result_get_info_memory(void *result, cJSON **json) {
    rpc_result_get_info_memory_t *info = result;
    *json = cJSON_CreateObject();
    cJSON *tasks = cJSON_CreateArray();
    for (uint32_t i = 0; i < info->num_tasks; ++i) {
        cJSON *task = cJSON_CreateObject();
        cJSON_AddStringToObject(task, "name", info->task_status[i].pcTaskName);
        cJSON_AddNumberToObject(task, "name", info->task_status[i].usStackHighWaterMark);
        cJSON_AddItemToArray(tasks, task);
    }
    if (info->task_status) {
        free(info->task_status);
    }
    cJSON_AddItemToObject(*json, "tasks", tasks);
    cJSON *heap = cJSON_CreateObject();
    cJSON_AddNumberToObject(heap, "allocated", info->heap.total_allocated_bytes);
    cJSON_AddNumberToObject(heap, "free", info->heap.total_free_bytes);
    cJSON_AddNumberToObject(heap, "minimum-free", info->heap.minimum_free_bytes);
    cJSON_AddItemToObject(*json, "heap", heap);
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

uint8_t rpc_json_result_get_info_sdcard(void *result, cJSON **json) {
    return RPC_ERROR_DIRECTORY_NOT_FOUND;
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

uint8_t rpc_json_result_get_file_list(void *result, cJSON **json) {
    rpc_result_get_file_list_t *file_list = result;
    uint8_t error = file_list->error;
    if (error == RPC_ERROR_NO_ERROR) {
        *json = cJSON_CreateObject();
        cJSON_AddStringToObject(*json, "path", file_list->path);
        if (file_list->entries.cover) {
            cJSON_AddStringToObject(*json, "cover", file_list->entries.cover);
            free(file_list->entries.cover);
        }
        if (file_list->entries.dirs) {
            cJSON *dirs = cJSON_CreateArray();
            entry_t *dptr = file_list->entries.dirs;
            while (dptr) {
                cJSON *dir = cJSON_CreateString(dptr->name);
                cJSON_AddItemToArray(dirs, dir);
                entry_t *nextptr = dptr->next;
                free(dptr->name);
                free(dptr);
                dptr = nextptr;
            }
            cJSON_AddItemToObject(*json, "dirs", dirs);
        }
        if (file_list->entries.tracks) {
            cJSON *tracks = cJSON_CreateArray();
            entry_t *tptr = file_list->entries.tracks;
            while (tptr) {
                cJSON *track = cJSON_CreateString(tptr->name);
                cJSON_AddItemToArray(tracks, track);
                entry_t *nextptr = tptr->next;
                free(tptr->name);
                free(tptr);
                tptr = nextptr;
            }
            cJSON_AddItemToObject(*json, "tracks", tracks);
        }
        free(file_list->path);
    }
    free(result);
    return error;
}

uint8_t rpc_json_result_get_track_info(void *result, cJSON **json) {
    rpc_result_get_track_info_t *info = result;
    uint8_t error = info->error;
    if (error == RPC_ERROR_NO_ERROR) {
        *json = cJSON_CreateObject();
        if (info->filename) {
            cJSON_AddStringToObject(*json, "file", info->filename);
            free(info->filename);
        }
        if (info->info.genre) {
            cJSON_AddStringToObject(*json, "genre", info->info.genre);
            free(info->info.genre);
        }
        if (info->info.artist) {
            cJSON_AddStringToObject(*json, "artist", info->info.artist);
            free(info->info.artist);
        }
        if (info->info.album) {
            cJSON_AddStringToObject(*json, "album", info->info.album);
            free(info->info.album);
        }
        if (info->info.title) {
            cJSON_AddStringToObject(*json, "title", info->info.title);
            free(info->info.title);
        }
        if (info->info.date) {
            cJSON_AddNumberToObject(*json, "date", info->info.date);
        }
        if (info->info.track) {
            cJSON_AddNumberToObject(*json, "track", info->info.track);
        }
        if (info->info.duration) {
            cJSON_AddNumberToObject(*json, "duration", info->info.duration);
        }
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

void *rpc_json_params_get_file_list(cJSON *params) {
    rpc_params_get_file_list_t *obj = NULL;
    if (cJSON_IsObject(params)) {
        cJSON *path = cJSON_GetObjectItemCaseSensitive(params, "path");
        if (   cJSON_IsString(path)
            && (strlen(path->valuestring) > 0)) {
            obj = calloc(1, sizeof(rpc_params_get_file_list_t));
            obj->path = strdup(path->valuestring);
        }
    } else if (!params) {
        obj = calloc(1, sizeof(rpc_params_get_file_list_t));
    }
    return obj;
}

void *rpc_json_params_get_track_info(cJSON *params) {
    rpc_params_get_track_info_t *obj = NULL;
    if (cJSON_IsObject(params)) {
        cJSON *file = cJSON_GetObjectItemCaseSensitive(params, "file");
        if (   cJSON_IsString(file)
            && (strlen(file->valuestring) > 0)) {
            obj = calloc(1, sizeof(rpc_params_get_track_info_t));
            obj->filename = strdup(file->valuestring);
        }
    }
    return obj;
}

void *rpc_json_params_set_volume(cJSON *params) {
    rpc_params_set_volume_t *obj = NULL;
    if (cJSON_IsObject(params)) {
        cJSON *left = cJSON_GetObjectItemCaseSensitive(params, "left");
        cJSON *right = cJSON_GetObjectItemCaseSensitive(params, "right");
        if (   cJSON_IsNumber(left)
            && cJSON_IsNumber(right)
            && left->valueint >= -127
            && left->valueint <= 0
            && right->valueint >= -127
            && right->valueint <= 0) {
            obj = calloc(1, sizeof(rpc_params_set_volume_t));
            obj->left = left->valueint;
            obj->right = right->valueint;
        }
    }
    return obj;
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/
