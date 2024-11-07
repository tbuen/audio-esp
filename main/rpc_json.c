#include <stdlib.h>

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

/***************************
***** LOCAL FUNCTIONS ******
***************************/
