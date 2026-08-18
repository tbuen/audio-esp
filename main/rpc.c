#include "json_rpc.h"
#include "rpc_types.h"
#include "rpc_handler.h"
#include "rpc_json.h"
#include "rpc.h"

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

static const json_rpc_config_t rpc_config[] = {
    { "get-info-connection"  , &rpc_handler_get_info_connection  , NULL                                , &rpc_json_result_get_info_connection   },
    { "get-info-about"       , &rpc_handler_get_info_about       , NULL                                , &rpc_json_result_get_info_about        },
    { "get-info-memory"      , &rpc_handler_get_info_memory      , NULL                                , &rpc_json_result_get_info_memory       },
    { "get-info-spiflash"    , &rpc_handler_get_info_spiflash    , NULL                                , &rpc_json_result_get_info_spiflash     },
    { "get-info-sdcard"      , &rpc_handler_get_info_sdcard      , NULL                                , &rpc_json_result_get_info_sdcard       },
    { "get-wifi-scan-result" , &rpc_handler_get_wifi_scan_result , NULL                                , &rpc_json_result_get_wifi_scan_result  },
    { "get-wifi-network-list", &rpc_handler_get_wifi_network_list, NULL                                , &rpc_json_result_get_wifi_network_list },
    { "set-wifi-network"     , &rpc_handler_set_wifi_network     , &rpc_json_params_set_wifi_network   , &rpc_json_result_error                 },
    { "delete-wifi-network"  , &rpc_handler_delete_wifi_network  , &rpc_json_params_delete_wifi_network, &rpc_json_result_error                 },
    { "get-file-list"        , &rpc_handler_get_file_list        , &rpc_json_params_get_file_list      , &rpc_json_result_get_file_list         },
    { "get-track-info"       , &rpc_handler_get_track_info       , &rpc_json_params_get_track_info     , &rpc_json_result_get_track_info        },
    { NULL                   , NULL                              , NULL                                , NULL                                   }
};

static const json_rpc_error_config_t rpc_err_config[] = {
    { RPC_ERROR_NOT_ALLOWED_IN_STA_MODE, "not allowed in STA mode" },
    { RPC_ERROR_NO_SPACE_LEFT          , "no space left"           },
    { RPC_ERROR_NETWORK_NOT_FOUND      , "network not found"       },
    { RPC_ERROR_DIRECTORY_NOT_FOUND    , "directory not found"     },
    { RPC_ERROR_FILE_NOT_FOUND         , "file not found"          },
    { RPC_ERROR_NO_TRACK               , "file is not a track"     },
    { RPC_ERROR_IO_ERROR               , "I/O error"               },
    { RPC_ERROR_NO_ERROR               , NULL                      }
};

/***************************
***** PUBLIC FUNCTIONS *****
***************************/

void rpc_init(void) {
    json_rpc_init(rpc_config, rpc_err_config);
}

char *rpc_handle_request(con_id_t con, const char *request) {
    return json_rpc_handle_request((void*)con, request);
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/

/*char *json_get_volume(void) {
    volume_t vol;
    audio_volume(&vol, false);
    char *string;

    cJSON *resp = cJSON_CreateObject();

    cJSON_AddNumberToObject(resp, "left", vol.left);
    cJSON_AddNumberToObject(resp, "right", vol.right);

    string = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);

    return string;
}

bool json_post_volume(const char *content, char **response) {
    bool valid = false;

    cJSON *req = cJSON_Parse(content);

    if (req) {
        cJSON *left = cJSON_GetObjectItemCaseSensitive(req, "left");
        cJSON *right = cJSON_GetObjectItemCaseSensitive(req, "right");
        if (cJSON_IsNumber(left) && cJSON_IsNumber(right)) {
            valid = true;

            volume_t vol = {
                .left = left->valueint,
                .right = right->valueint,
            };
            audio_volume(&vol, true);

            cJSON *resp = cJSON_CreateObject();

            cJSON_AddNumberToObject(resp, "left", vol.left);
            cJSON_AddNumberToObject(resp, "right", vol.right);

            *response = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
        }
        cJSON_Delete(req);
    }

    return valid;
}

bool json_post_play(const char *content, char **response) {
    bool valid = false;

    cJSON *req = cJSON_Parse(content);

    if (req) {
        cJSON *filename = cJSON_GetObjectItemCaseSensitive(req, "filename");
        if (cJSON_IsString(filename) && filename->valuestring) {
            valid = true;

            bool status = audio_play(filename->valuestring);

            cJSON *resp = cJSON_CreateObject();

            cJSON_AddBoolToObject(resp, "status", status);

            *response = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
        }
        cJSON_Delete(req);
    }

    return valid;
}*/
