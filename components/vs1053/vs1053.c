#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"

#include "defs.h"
#include "sci.h"
#include "vs1053.h"

static const char *TAG = "vs1053";

static spi_device_handle_t sci_handle;
static bool initialized;

void vs_init(void) {
    if (initialized) return;

    gpio_config_t io_conf = {};

    io_conf.pin_bit_mask = BIT64(PIN_RESET);
    io_conf.mode = GPIO_MODE_OUTPUT;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = BIT64(PIN_DREQ);
    io_conf.mode = GPIO_MODE_INPUT;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(PIN_RESET, 0);

    spi_bus_config_t bus_conf = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO | SPICOMMON_BUSFLAG_MOSI,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HOST, &bus_conf, SPI_DMA_DISABLED));

    spi_device_interface_config_t sci_conf = {
        .command_bits = 8,
        .address_bits = 8,
        .clock_speed_hz = SCI_FREQ,
        .spics_io_num = PIN_SCI_CS,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = 5,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(HOST, &sci_conf, &sci_handle));

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

    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_RESET, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    sci_write(sci_handle, REG_MODE, MODE_SDINEW | MODE_RESET);
    vTaskDelay(100 / portTICK_PERIOD_MS);

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

void vs_card_open(void) {
    if (!initialized) return;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdmmc_card_t card;
    if (sdmmc_card_init(&host, &card) == ESP_OK) {
        ESP_LOGI(TAG, "card ok");
    } else {
        ESP_LOGW(TAG, "no card");
    }
}
