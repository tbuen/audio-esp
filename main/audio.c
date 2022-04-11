#include "errno.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "string.h"
#include "sys/dirent.h"

#include "vs1053.h"
#include "audio.h"

#define TASK_PRIO      0
#define STACK_SIZE  4096

static const char *TAG = "audio";

static SemaphoreHandle_t mutex;
static TaskHandle_t handle;

static audio_track *tracks;

static void audio_task(void *param);
static esp_err_t read_dir(const char *path, audio_track **tracks);

void audio_init(void) {
    if (handle) return;

    mutex = xSemaphoreCreateMutex();

    if (xTaskCreate(&audio_task, "audio-task", STACK_SIZE, NULL, TASK_PRIO, &handle) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

const audio_track *audio_get_tracks(void) {
    audio_track *t = NULL;
    if (xSemaphoreTake(mutex, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
        t = tracks;
    }
    return t;
}

void audio_release_tracks(void) {
    xSemaphoreGive(mutex);
}

static void audio_task(void *param) {
    const char *path = "/sdcard";
    esp_err_t res = ESP_FAIL;
    vs_init();
    vs_set_volume(0xA0, 0xC0);

    for (;;) {
        if (res == ESP_FAIL) {
            res = vs_card_init(path);
            if (res == ESP_OK) {
                if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
                    res = read_dir(path, &tracks);
                    xSemaphoreGive(mutex);
                }
            }
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

static esp_err_t read_dir(const char *path, audio_track **tracks) {
    while (*tracks) {
        audio_track *tmp = *tracks;
        *tracks = tmp->next;
        free(tmp->filename);
        free(tmp);
    }

    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "error opening directory");
        return ESP_FAIL;
    }

    struct dirent *entry;
    do {
        errno = 0;
        entry = readdir(dir);
        if (errno) {
            ESP_LOGE(TAG, "error reading directory");
            return ESP_FAIL;
        }
        if (entry) {
            ESP_LOGI(TAG, "File found: %s", entry->d_name);
            if (strstr(entry->d_name, ".OGG")) {
                *tracks = calloc(1, sizeof(audio_track));
                asprintf(&(*tracks)->filename, "%s/%s", path, entry->d_name);
                tracks = &(*tracks)->next;
            }
        }
    } while (entry);

    closedir(dir);

    return ESP_OK;
}
