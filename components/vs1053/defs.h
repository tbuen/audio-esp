#pragma once

#define HOST        SPI2_HOST

#define PIN_RESET   GPIO_NUM_13
#define PIN_DREQ    GPIO_NUM_34
#define PIN_MOSI    GPIO_NUM_27
#define PIN_MISO    GPIO_NUM_26
#define PIN_CLK     GPIO_NUM_12
#define PIN_SCI_CS  GPIO_NUM_14
#define PIN_SD_CS   GPIO_NUM_25

#define SCI_FREQ     100000

#define CMD_WRITE      0x02
#define CMD_READ       0x03

#define REG_MODE       0x00
#define REG_STATUS     0x01
#define REG_VOL        0x0B

#define MODE_SDINEW  0x0800
#define MODE_RESET   0x0004
