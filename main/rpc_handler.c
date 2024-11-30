#include <esp_app_desc.h>
#include <string.h>

#include "filesystem.h"
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

void rpc_handler_get_version(void *params, void **result) {
    rpc_result_get_version_t *version = calloc(1, sizeof(rpc_result_get_version_t));
    const esp_app_desc_t *descr = esp_app_get_description();
    strncpy(version->project, descr->project_name, sizeof(version->project));
    strncpy(version->version, descr->version, sizeof(version->version));
    strncpy(version->idf_ver, descr->idf_ver, sizeof(version->idf_ver));
    strncpy(version->date, descr->date, sizeof(version->date));
    strncpy(version->time, descr->time, sizeof(version->time));
    *result = version;
}

void rpc_handler_get_info_spiflash(void *params, void **result) {
    rpc_result_get_info_spiflash_t *info = calloc(1, sizeof(rpc_result_get_info_spiflash_t));
    fs_web_info(info);
    *result = info;
}

void rpc_handler_get_wifi_scan_result(void *params, void **result) {
    rpc_result_get_wifi_scan_result_t *scan_result = calloc(1, sizeof(rpc_result_get_wifi_scan_result_t));
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
    *result = scan_result;
}

void rpc_handler_get_wifi_network_list(void *params, void **result) {
    rpc_result_get_wifi_network_list_t *network_list = calloc(1, sizeof(rpc_result_get_wifi_network_list_t));
    fs_wifi_cfg_t *cfg = fs_get_wifi_cfg();
    int j = 0;
    for (int i = 0; (i < FS_NUMBER_OF_WIFI_NETWORKS) && (j < RPC_WIFI_NUMBER_OF_NETWORKS); ++i) {
        if (cfg->network[i].ssid[0]) {
            memcpy(network_list->network[j].ssid, cfg->network[i].ssid, sizeof(cfg->network[i].ssid));
            j++;
        }
    }
    fs_free_wifi_cfg(false);
    *result = network_list;
}

void rpc_handler_set_wifi_network(void *params, void **result) {
    rpc_params_set_wifi_network_t *network = params;
    rpc_result_error_t *result_obj = calloc(1, sizeof(rpc_result_error_t));
    fs_wifi_cfg_t *cfg = fs_get_wifi_cfg();
    int idx = -1;
    for (int i = 0; i < FS_NUMBER_OF_WIFI_NETWORKS; ++i) {
        if ((idx == -1) && (!cfg->network[i].ssid[0])) {
            idx = i;
        } else if (!memcmp(cfg->network[i].ssid, network->ssid, sizeof(network->ssid))) {
            idx = i;
            break;
        }
    }
    if (idx > -1) {
        memcpy(cfg->network[idx].ssid, network->ssid, sizeof(network->ssid));
        memcpy(cfg->network[idx].key, network->key, sizeof(network->ssid));
        fs_free_wifi_cfg(true);
        result_obj->error = RPC_ERROR_NO_ERROR;
    } else {
        fs_free_wifi_cfg(false);
        result_obj->error = RPC_ERROR_NO_SPACE_LEFT;
    }
    free(params);
    *result = result_obj;
}

void rpc_handler_delete_wifi_network(void *params, void **result) {
    rpc_params_delete_wifi_network_t *network = params;
    rpc_result_error_t *result_obj = calloc(1, sizeof(rpc_result_error_t));
    fs_wifi_cfg_t *cfg = fs_get_wifi_cfg();
    int idx = -1;
    for (int i = 0; i < FS_NUMBER_OF_WIFI_NETWORKS; ++i) {
        if (!memcmp(cfg->network[i].ssid, network->ssid, sizeof(network->ssid))) {
            idx = i;
            break;
        }
    }
    if (idx > -1) {
        memset(&cfg->network[idx], 0, sizeof(cfg->network[idx]));
        fs_free_wifi_cfg(true);
        result_obj->error = RPC_ERROR_NO_ERROR;
    } else {
        fs_free_wifi_cfg(false);
        result_obj->error = RPC_ERROR_NOT_FOUND;
    }
    free(params);
    *result = result_obj;
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/
