//#include <stdio.h>
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "vs1053.h"

#define PIN_RESET GPIO_NUM_13
#define PIN_DREQ  GPIO_NUM_34
#define PIN_MOSI  GPIO_NUM_27
#define PIN_MISO  GPIO_NUM_26
#define PIN_CLK   GPIO_NUM_12
#define PIN_CS    GPIO_NUM_14

#define PIN_MASK(p) ((uint64_t)1 << (p))

#define SPI_FREQ  100000

#define CMD_WRITE 0x02
#define CMD_READ  0x03

#define REG_MODE   0x00
#define REG_STATUS 0x01

#define MODE_SDINEW 0x0800
#define MODE_RESET  0x0004

static const char *TAG = "vs1053";

static spi_device_handle_t handle = NULL;

static esp_err_t sci_write(uint8_t addr, uint16_t data);
static esp_err_t sci_read(uint8_t addr, uint16_t *data);

esp_err_t vs1053_init(void) {
    gpio_config_t io_conf = {};

    io_conf.pin_bit_mask = PIN_MASK(PIN_RESET);
    io_conf.mode = GPIO_MODE_OUTPUT;
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "gpio_config() failed");

    gpio_set_level(PIN_RESET, 0);

    io_conf.pin_bit_mask = PIN_MASK(PIN_DREQ);
    io_conf.mode = GPIO_MODE_INPUT;
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "gpio_config() failed");

    spi_bus_config_t bus_conf = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO | SPICOMMON_BUSFLAG_MOSI,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus_conf, SPI_DMA_DISABLED), TAG, "spi_bus_initialize() failed");

    spi_device_interface_config_t dev_conf = {
        .command_bits = 8,
        .address_bits = 8,
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = SPI_FREQ,
        .input_delay_ns = 0,
        .spics_io_num = PIN_CS,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = 5,
        .pre_cb = NULL,
        .post_cb = NULL,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &dev_conf, &handle), TAG, "spi_bus_add_device() failed");

    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_RESET, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    ESP_RETURN_ON_ERROR(sci_write(REG_MODE, MODE_SDINEW | MODE_RESET), TAG, "sci_write() failed");

    vTaskDelay(100 / portTICK_PERIOD_MS);

    uint16_t status = 0;
    ESP_RETURN_ON_ERROR(sci_read(REG_STATUS, &status), TAG, "sci_read() failed");
    ESP_LOGD(TAG, "STATUS = 0x%04x\n", status);

    ESP_RETURN_ON_FALSE(((status & 0xF0) >> 4) == 4, ESP_ERR_INVALID_VERSION, TAG, "invalid version");

    ESP_LOGI(TAG, "init ok");

    return ESP_OK;
}

static esp_err_t sci_write(uint8_t addr, uint16_t data) {
    while (gpio_get_level(PIN_DREQ) != 1) {
        vTaskDelay(1);
    }
    spi_transaction_t trans = {
        .flags = SPI_TRANS_USE_TXDATA,
        .cmd = CMD_WRITE,
        .addr = addr,
        .length = 16,
        .tx_data = {data >> 8, data = 0xFF},
    };
    ESP_RETURN_ON_ERROR(spi_device_transmit(handle, &trans), TAG, "spi_device_transmit() failed");
    return ESP_OK;
}

static esp_err_t sci_read(uint8_t addr, uint16_t *data) {
    while (gpio_get_level(PIN_DREQ) != 1) {
        vTaskDelay(1);
    }
    spi_transaction_t trans = {
        .flags = SPI_TRANS_USE_RXDATA,
        .cmd = CMD_READ,
        .addr = addr,
        .rxlength = 16,
    };
    ESP_RETURN_ON_ERROR(spi_device_transmit(handle, &trans), TAG, "spi_device_transmit() failed");
    *data = trans.rx_data[0] << 8 | trans.rx_data[1];
    return ESP_OK;
}
