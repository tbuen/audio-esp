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

#include "message.h"
#include "vs1053.h"
#include "audio.h"

#define TASK_CORE      1
#define TASK_PRIO     19
#define STACK_SIZE  4096

#define MOUNT_POINT "/sdcard"

static const char *TAG = "audio";

typedef enum {
    REQ_GET_FILE_LIST
} req_t;

typedef struct {
    req_t req;
    void *ctx;
} req_msg_t;

static TaskHandle_t handle;
static QueueHandle_t queue;
static QueueHandle_t request_queue;
static SemaphoreHandle_t mutex;

static char *file2play;

static void audio_task(void *param);
static audio_error_t read_dir(const char *path, audio_file_list_t *list);

void audio_init(QueueHandle_t q) {
    if (handle) return;

    queue = q;

    mutex = xSemaphoreCreateMutex();

    request_queue = xQueueCreate(5, sizeof(req_msg_t));

    if (xTaskCreatePinnedToCore(&audio_task, "audio-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

void audio_get_file_list(void *ctx) {
    req_msg_t req = { REQ_GET_FILE_LIST, ctx };
    xQueueSendToBack(request_queue, &req, 0);
}

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
    esp_err_t res = ESP_FAIL;
    req_msg_t msg;

    vs_init();
    vs_set_volume(0x1414);

    for (;;) {
        if (xQueueReceive(request_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (res == ESP_FAIL) {
                res = vs_card_init(MOUNT_POINT);
            }

            // TODO: repeat if fails for the first time
            if (msg.req == REQ_GET_FILE_LIST) {
                if (res == ESP_OK) {
                    msg_audio_file_list_t *msg_data = calloc(1, sizeof(msg_audio_file_list_t));
                    msg_data->ctx = msg.ctx;
                    msg_data->error = read_dir(MOUNT_POINT, &msg_data->file_list);
                    message_t msg = { BASE_AUDIO, EVENT_AUDIO_FILE_LIST, msg_data };
                    xQueueSendToBack(queue, &msg, 0);
                }
            }

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

static audio_error_t read_dir(const char *path, audio_file_list_t *list) {
    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "error opening directory");
        return ERROR_AUDIO_FAILURE;
    }

    audio_error_t res = ERROR_AUDIO_SUCCESS;
    struct dirent *entry;
    do {
        errno = 0;
        entry = readdir(dir);
        if (errno) {
            ESP_LOGE(TAG, "error reading directory");
            res = ERROR_AUDIO_FAILURE;
        }
        if (entry) {
            if (entry->d_type == DT_REG) {
                ESP_LOGI(TAG, "File found: %s", entry->d_name);
                if (list->cnt < FILE_LIST_SIZE) {
                    asprintf(&list->files[list->cnt].name, "%s/%s", &path[strlen(MOUNT_POINT)], entry->d_name);
                    list->cnt++;
                } else {
                    res = ERROR_AUDIO_LIST_FULL;
                    break;
                }
            } else if (entry->d_type == DT_DIR) {
                char *p;
                asprintf(&p, "%s/%s", path, entry->d_name);
                res = read_dir(p, list);
                free(p);
            }
        }
    } while (res == ERROR_AUDIO_SUCCESS && entry);

    closedir(dir);

    return res;
}
