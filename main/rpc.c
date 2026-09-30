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
    { "set-volume"           , &rpc_handler_set_volume           , &rpc_json_params_set_volume         , &rpc_json_result_error                 },
    { NULL                   , NULL                              , NULL                                , NULL                                   }
};

static const json_rpc_error_config_t rpc_err_config[] = {
    { RPC_ERROR_NOT_ALLOWED_IN_STA_MODE, "not allowed in STA mode"       },
    { RPC_ERROR_NO_SPACE_LEFT          , "no space left"                 },
    { RPC_ERROR_NETWORK_NOT_FOUND      , "network not found"             },
    { RPC_ERROR_DIRECTORY_NOT_FOUND    , "directory not found"           },
    { RPC_ERROR_FILE_NOT_FOUND         , "file not found"                },
    { RPC_ERROR_NO_TRACK               , "file is not a track"           },
    { RPC_ERROR_IO_ERROR               , "I/O error"                     },
    { RPC_ERROR_VOLUME_OUT_OF_RANGE    , "volume out of range [-127..0]" },
    { RPC_ERROR_NO_ERROR               , NULL                            }
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
