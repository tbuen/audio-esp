#pragma once

#include "esp_err.h"

esp_err_t vs_init(void);

void vs_set_volume(uint8_t left, uint8_t right);

esp_err_t vs_card_init(const char *mountpoint);
