#pragma once

void vs_init(void);

void vs_set_volume(uint8_t left, uint8_t right);

bool vs_card_open(void);
bool vs_read_dir(void);
