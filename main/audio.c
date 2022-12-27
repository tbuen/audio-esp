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

#define MSG_FILES      1

static const char *TAG = "audio";

static SemaphoreHandle_t mutex;
static QueueHandle_t request_queue;
static QueueHandle_t response_queue;
static TaskHandle_t handle;

static char *file2play;

static void audio_task(void *param);
static esp_err_t read_dir(const char *path, file_t **files);

void audio_init(void) {
    if (handle) return;

    mutex = xSemaphoreCreateMutex();

    request_queue = xQueueCreate(5, sizeof(uint32_t));
    response_queue = xQueueCreate(5, sizeof(uint32_t));

    if (xTaskCreatePinnedToCore(&audio_task, "audio-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

const file_t *audio_get_files(void) {
    uint32_t msg = MSG_FILES;

    if (   (xQueueSendToBack(request_queue, &msg, 0) == pdTRUE)
        && (xQueueReceive(response_queue, &msg, portMAX_DELAY) == pdTRUE)) {
        return (file_t *)msg;
    }

    return NULL;
}

void audio_free_files(file_t *file) {
    while (file) {
        file_t *tmp = file;
        file = tmp->next;
        free(tmp->name);
        free(tmp);
    }
}

// TODO audio_free_files()

// TODO call vs_* only in task
void audio_volume(volume_t *vol, bool set) {
    if (set) {
        if (vol->left > 0) vol->left = 0;
        if (vol->left < -127) vol->left = -127;
        if (vol->right > 0) vol->right = 0;
        if (vol->right < -127) vol->right = -127;

        uint8_t left = -2 * vol->left;
        uint8_t right = -2 * vol->right;
        vs_set_volume((left << 8) | right);
    }
    uint16_t v = vs_get_volume();
    vol->left = (v >> 8) / -2;
    vol->right = (v & 0xFF) / -2;
}

bool audio_play(const char *filename) {
    if (!file2play) {
        file2play = strdup(filename);
        return true;
    }
    return false;
}

//static uint8_t data[4096];

static void audio_task(void *param) {
    const char *path = "/sdcard";
    esp_err_t res = ESP_FAIL;
    uint32_t msg;

    vs_init();
    vs_set_volume(0x1414);

    for (;;) {
        if (xQueueReceive(request_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (res == ESP_FAIL) {
                res = vs_card_init(path);
            }

            uint32_t response = 0;

            // TODO: repeat if fails for the first time
            if (msg == MSG_FILES) {
                if (res == ESP_OK) {
                    file_t *files;
                    res = read_dir(path, &files);
                    response = (uint32_t)files;
                }
            }

            xQueueSendToBack(response_queue, &response, 0);
        }

        /*if (res == ESP_OK) {
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
        }*/
        //vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static esp_err_t read_dir(const char *path, file_t **files) {
    *files = NULL;

    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "error opening directory");
        return ESP_FAIL;
    }

    esp_err_t res = ESP_OK;
    struct dirent *entry;
    do {
        errno = 0;
        entry = readdir(dir);
        if (errno) {
            ESP_LOGE(TAG, "error reading directory");
            res = ESP_FAIL;
        }
        if (entry) {
            if (entry->d_type == DT_REG) {
                ESP_LOGI(TAG, "File found: %s", entry->d_name);
                *files = calloc(1, sizeof(file_t));
                asprintf(&(*files)->name, "%s/%s", path, entry->d_name);
                files = &(*files)->next;
            } else if (entry->d_type == DT_DIR) {
                char *p;
                asprintf(&p, "%s/%s", path, entry->d_name);
                res = read_dir(p, files);
                while (*files) {
                    files = &(*files)->next;
                }
                free(p);
            }
        }
    } while (res == ESP_OK && entry);

    closedir(dir);

    return res;
}
