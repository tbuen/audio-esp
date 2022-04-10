#include "diskio_impl.h"
#include "diskio_sdmmc.h"
#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"
#include "sys/dirent.h"

#include "defs.h"
#include "sci.h"
#include "vs1053.h"

static const char *TAG = "vs1053";

static spi_device_handle_t sci_handle;
static bool initialized;
static sdmmc_card_t card;
static bool cardOk;

void vs_init(void) {
    if (initialized) return;

    // configure pins
    gpio_config_t io_conf = {};

    io_conf.pin_bit_mask = BIT64(PIN_RESET);
    io_conf.mode = GPIO_MODE_OUTPUT;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = BIT64(PIN_DREQ);
    io_conf.mode = GPIO_MODE_INPUT;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // reset active
    gpio_set_level(PIN_RESET, 0);

    // init spi
    spi_bus_config_t bus_conf = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HOST, &bus_conf, SPI_DMA_CH_AUTO));

    // add sci device to spi bus
    spi_device_interface_config_t sci_conf = {
        .command_bits = 8,
        .address_bits = 8,
        .clock_speed_hz = SCI_FREQ,
        .spics_io_num = PIN_SCI_CS,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = 5,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(HOST, &sci_conf, &sci_handle));

    // add sd card device to spi bus
    ESP_ERROR_CHECK(sdspi_host_init());
    sdspi_device_config_t sd_conf = {
        .host_id = HOST,
        .gpio_cs = PIN_SD_CS,
        .gpio_cd = SDSPI_SLOT_NO_CD,
        .gpio_wp = SDSPI_SLOT_NO_WP,
        .gpio_int = SDSPI_SLOT_NO_INT,
    };
    sdspi_dev_handle_t sd_handle;
    ESP_ERROR_CHECK(sdspi_host_init_device(&sd_conf, &sd_handle));

    // wake from reset
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_RESET, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    sci_write(sci_handle, REG_MODE, MODE_SDINEW | MODE_RESET);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // read status
    uint16_t status = sci_read(sci_handle, REG_STATUS);
    ESP_LOGD(TAG, "status: 0x%04x\n", status);
    if (((status & 0xF0) >> 4) == 4) {
        ESP_LOGI(TAG, "init ok");
        initialized = true;
    } else {
        ESP_LOGE(TAG, "invalid version");
    }
}

void vs_set_volume(uint8_t left, uint8_t right) {
    if (!initialized) return;

    ESP_LOGI(TAG, "set volume to %1.1fdB %1.1fdB", -0.5 * left, -0.5 * right);
    sci_write(sci_handle, REG_VOL, (left << 8) | right);
}

bool vs_card_open(void) {
    if (!initialized) return false;

    const char drive[] = { '0' + FS_NUM, ':', 0 };

    if (cardOk) {
        f_unmount("1:");
        ff_diskio_register(FS_NUM, NULL);
        esp_vfs_fat_unregister_path(MOUNTPOINT);
        cardOk = false;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    if (   (sdmmc_card_init(&host, &card) != ESP_OK)
        || (sdmmc_get_status(&card) != ESP_OK)) {
        ESP_LOGW(TAG, "no card");
        return false;
    }

    FATFS *fatfs = NULL;
    ESP_ERROR_CHECK(esp_vfs_fat_register(MOUNTPOINT, drive, MAX_OPEN_FILES, &fatfs));

    ff_diskio_register_sdmmc(FS_NUM, &card);

    FRESULT fres = f_mount(fatfs, drive, 1);
    if (fres != FR_OK) {
        ESP_LOGE(TAG, "mount failed: %d", fres);
        ff_diskio_register(FS_NUM, NULL);
        esp_vfs_fat_unregister_path(MOUNTPOINT);
        return false;
    }

    ESP_LOGI(TAG, "card ok");
    cardOk = true;
    return true;
}

bool vs_read_dir(void) {
    if (!initialized) return false;

    if (sdmmc_get_status(&card) != ESP_OK) {
        ESP_LOGE(TAG, "card not ok");
        return false;
    }

    DIR *dir = opendir(MOUNTPOINT);
    if (!dir) {
        ESP_LOGE(TAG, "reading directory failed");
        return false;
    }

    struct dirent *entry = readdir(dir);
    if (!entry) {
        ESP_LOGW(TAG, "empty directory");
        return false;
    }
    while (entry) {
        ESP_LOGI(TAG, "File found: %s", entry->d_name);
        entry = readdir(dir);
    }
    closedir(dir);

    return true;
}
