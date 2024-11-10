#include <stdlib.h>

#include "cJSON.h"
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

cJSON *rpc_json_result_get_version(void *result) {
    cJSON *json = cJSON_CreateObject();
    rpc_result_get_version_t *version = result;
    cJSON_AddStringToObject(json, "project", version->project);
    cJSON_AddStringToObject(json, "version", version->version);
    cJSON_AddStringToObject(json, "esp-idf", version->idf_ver);
    cJSON_AddStringToObject(json, "date", version->date);
    cJSON_AddStringToObject(json, "time", version->time);
    free(result);
    return json;
}

cJSON *rpc_json_result_get_wifi_scan_result(void *result) {
    cJSON *json = cJSON_CreateArray();
    rpc_result_get_wlan_scan_result_t *scan_result = result;
    for (int i = 0; i < scan_result->cnt; ++i) {
        cJSON *ap = cJSON_CreateObject();
        cJSON_AddStringToObject(ap, "ssid", scan_result->ap[i].ssid);
        cJSON_AddNumberToObject(ap, "rssi", scan_result->ap[i].rssi);
        cJSON_AddItemToArray(json, ap);
    }
    free(result);
    return json;
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/
