#include "errno.h"
#include "esp_log.h"
#include "fcntl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "string.h"
#include "sys/dirent.h"
#include "sys/stat.h"
#include "unistd.h"

#include "vs1053.h"
#include "audio.h"

#define TASK_CORE      1
#define TASK_PRIO     19
#define STACK_SIZE  4096

static const char *TAG = "audio";

static SemaphoreHandle_t mutex;
static TaskHandle_t handle;

static audio_track *tracks;
static char *file2play;

static void audio_task(void *param);
static esp_err_t read_dir(const char *path, audio_track **tracks);

void audio_init(void) {
    if (handle) return;

    mutex = xSemaphoreCreateMutex();

    if (xTaskCreatePinnedToCore(&audio_task, "audio-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

const audio_track *audio_get_tracks(void) {
    audio_track *t = NULL;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000))) {
        t = tracks;
    }
    return t;
}

void audio_release_tracks(void) {
    xSemaphoreGive(mutex);
}

bool audio_play(const char *filename) {
    if (!file2play) {
        file2play = strdup(filename);
        return true;
    }
    return false;
}

static uint8_t data[4096];

static void audio_task(void *param) {
    const char *path = "/sdcard";
    esp_err_t res = ESP_FAIL;
    vs_init();
    vs_set_volume(0x20, 0x20);

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
        if (res == ESP_OK) {
            if (file2play) {
                // start playing
                struct stat st;
                if (stat(file2play, &st) == 0) {
                    ESP_LOGI(TAG, "Play file %s, size %ld", file2play, st.st_size);
                    int fd = open(file2play, O_RDONLY, 0);
                    if (fd >= 0) {
                        int n;
                        while ((n = read(fd, data, sizeof(data))) > 0) {
                            //ESP_LOGI(TAG, "read %d bytes", n);
                            vs_send_data(data, n);
                            ESP_LOGI(TAG, "status: %s", vs_get_status());
                            taskYIELD();
                        }
                        if (n == 0) {
                            ESP_LOGI(TAG, "reached EOF!");
                        } else {
                            ESP_LOGI(TAG, "error reading file!");
                        }
                        close(fd);
                    }
                } else {
                    ESP_LOGI(TAG, "File file %s does not exist!", file2play);
                }
                free(file2play);
                file2play = NULL;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
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
