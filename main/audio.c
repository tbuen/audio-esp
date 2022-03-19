#include "esp_check.h"

#include "vs1053.h"

void audio_init(void) {
    ESP_ERROR_CHECK(vs1053_init());
}
