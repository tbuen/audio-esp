#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "vs1053.h"
#include "audio.h"

#define TASK_PRIO     0
#define STACK_SIZE 4096

static const char *TAG = "audio";

static TaskHandle_t handle;

static void audio_task(void *param);

void audio_init(void) {
    if (handle) return;

    if (xTaskCreate(&audio_task, "audio-task", STACK_SIZE, NULL, TASK_PRIO, &handle) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

static void audio_task(void *param) {
    vs_init();
    vs_set_volume(0xA0, 0xC0);
    vs_card_open();

    for (;;) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
