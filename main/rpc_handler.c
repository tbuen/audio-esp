#include <esp_app_desc.h>
#include <string.h>

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

/***************************
***** LOCAL FUNCTIONS ******
***************************/
