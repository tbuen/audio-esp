#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "nv.h"

// defines

#define FILESYSTEM_LABEL         "filesystem"
#define MOUNTPOINT               "/spiflash"
#define WIFI_CFG_FILE            "/spiflash/wificfg.bin"

// function prototypes

// local variables

static const char *TAG = "audio:nv";
static SemaphoreHandle_t mutex;
static nv_wifi_cfg_t wifi_cfg;

// public functions

void nv_init(void) {
    mutex = xSemaphoreCreateMutex();
    esp_vfs_fat_mount_config_t fat_config = {
        .format_if_mount_failed = true,
        .max_files = 1,
        .allocation_unit_size = 0
    };
    wl_handle_t wl_handle;
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount(MOUNTPOINT, FILESYSTEM_LABEL, &fat_config, &wl_handle));
    FILE *f = fopen(WIFI_CFG_FILE, "r");
    if (f) {
        fread(&wifi_cfg, sizeof(nv_wifi_cfg_t), 1, f);
        fclose(f);
    } else {
        ESP_LOGW(TAG, "could not open %s", WIFI_CFG_FILE);
    }
}

nv_wifi_cfg_t *nv_get_wifi_cfg(void) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    return &wifi_cfg;
}

void nv_free_wifi_cfg(bool save) {
    if (save) {
        FILE *f = fopen(WIFI_CFG_FILE, "w");
        if (f) {
            fwrite(&wifi_cfg, sizeof(nv_wifi_cfg_t), 1, f);
            fclose(f);
        } else {
            ESP_LOGW(TAG, "could not open %s", WIFI_CFG_FILE);
        }
    }
    xSemaphoreGive(mutex);
}

// local functions
