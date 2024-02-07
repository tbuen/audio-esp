#pragma once

#define NV_NUMBER_OF_WIFI_NETWORKS  5

typedef struct {
    uint8_t ssid[32];
    uint8_t key[64];
} nv_wifi_network_t;

typedef struct {
    nv_wifi_network_t networks[NV_NUMBER_OF_WIFI_NETWORKS];
} nv_wifi_cfg_t;

void nv_init(void);

nv_wifi_cfg_t *nv_get_wifi_cfg(void);

void nv_free_wifi_cfg(bool save);
