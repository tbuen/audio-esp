#include <esp_app_desc.h>
#include <string.h>

#include "rpc_types.h"
#include "rpc_handler.h"
#include "wlan.h"

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

void rpc_handler_get_wifi_scan_result(void *params, void **result) {
    rpc_result_get_wlan_scan_result_t *scan_result = calloc(1, sizeof(rpc_result_get_wlan_scan_result_t));
    uint8_t cnt;
    wlan_ap_t *ap;
    if (wlan_get_scan_result(&cnt, &ap)) {
        int j = 0;
        for (int i = 0; (i < cnt) && (j < RPC_WLAN_SCAN_RESULT_MAX_AP); ++i) {
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

/***************************
***** LOCAL FUNCTIONS ******
***************************/
