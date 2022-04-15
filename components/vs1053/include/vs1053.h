#pragma once

#include "esp_err.h"

esp_err_t vs_init(void);

void vs_set_volume(uint8_t left, uint8_t right);

void vs_send_data(const uint8_t *data, uint16_t len);

const char *vs_get_status(void);

esp_err_t vs_card_init(const char *mountpoint);
