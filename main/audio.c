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

#define MOUNT_POINT "/sdcard"
#define MAX_DEPTH   5

static const char *TAG = "audio";

typedef struct {
    com_ctx_t *com_ctx;
    audio_ctx_t *audio_ctx;
} req_msg_t;

typedef struct {
    char *path;
    DIR *dir;
} dir_t;

typedef struct {
    dir_t dir[MAX_DEPTH];
    uint8_t idx;
} ctx_data_t;

static TaskHandle_t handle;
static QueueHandle_t queue;
static QueueHandle_t request_queue;
static SemaphoreHandle_t mutex;

static char *file2play;

static void audio_task(void *param);
static esp_err_t read_dir(audio_ctx_t *audio_ctx, audio_file_list_t *list);

void audio_init(QueueHandle_t q) {
    if (handle) return;

    queue = q;

    mutex = xSemaphoreCreateMutex();

    request_queue = xQueueCreate(5, sizeof(req_msg_t));

    if (xTaskCreatePinnedToCore(&audio_task, "audio-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

void audio_request(com_ctx_t *com_ctx, audio_ctx_t *audio_ctx) {
    req_msg_t req = { com_ctx, audio_ctx };
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

            switch (msg.audio_ctx->request) {
                case AUDIO_REQ_FILE_LIST:
                    {
                        msg_audio_file_list_t *msg_data = calloc(1, sizeof(msg_audio_file_list_t));
                        msg_data->com_ctx = msg.com_ctx;
                        msg_data->audio_ctx = msg.audio_ctx;
                        if (res == ESP_OK) {
                            res = read_dir(msg_data->audio_ctx, &msg_data->list);
                        } else {
                            msg_data->audio_ctx->stop = true;
                        }
                        message_t msg = { BASE_AUDIO, EVENT_AUDIO_FILE_LIST, msg_data };
                        xQueueSendToBack(queue, &msg, 0);
                        break;
                    }
                default:
                    break;
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

static esp_err_t read_dir(audio_ctx_t *audio_ctx, audio_file_list_t *list) {
    esp_err_t error = ESP_OK;
    ctx_data_t *ctx = audio_ctx->data;

    if (audio_ctx->start) {
        ctx = calloc(1, sizeof(ctx_data_t));
        audio_ctx->data = ctx;
        asprintf(&ctx->dir[0].path, "%s", MOUNT_POINT);
        ctx->dir[0].dir = opendir(ctx->dir[0].path);
        if (!ctx->dir[0].dir) {
            ESP_LOGE(TAG, "error opening directory %s", ctx->dir[0].path);
            error = ESP_FAIL;
        }
    }
    char *path = ctx->dir[ctx->idx].path;
    DIR *dir = ctx->dir[ctx->idx].dir;

    ESP_LOGI(TAG, "start with idx %d and path %s", ctx->idx, ctx->dir[ctx->idx].path);

    struct dirent *entry;
    while (error == ESP_OK && !audio_ctx->stop) {
        errno = 0;
        entry = readdir(dir);
        if (errno) {
            ESP_LOGE(TAG, "error reading directory");
            error = ESP_FAIL;
        } else if (entry) {
            if (entry->d_type == DT_REG) {
                ESP_LOGI(TAG, "File found: %s", entry->d_name);
                asprintf(&list->files[list->cnt].name, "%s/%s", &path[strlen(MOUNT_POINT)], entry->d_name);
                list->cnt++;
                if (list->cnt == FILE_LIST_SIZE) {
                    ESP_LOGI(TAG, "list full -> break");
                    break;
                }
            } else if (entry->d_type == DT_DIR) {
                if (ctx->idx < MAX_DEPTH - 1) {
                    ctx->idx++;
                    asprintf(&ctx->dir[ctx->idx].path, "%s/%s", ctx->dir[ctx->idx-1].path, entry->d_name);
                    ctx->dir[ctx->idx].dir = opendir(ctx->dir[ctx->idx].path);
                    path = ctx->dir[ctx->idx].path;
                    dir = ctx->dir[ctx->idx].dir;
                    if (!dir) {
                        ESP_LOGE(TAG, "error opening directory %s", path);
                        error = ESP_FAIL;
                    }
                }
            }
        } else {
            ESP_LOGI(TAG, "no more files found in dir -> closing");
            closedir(dir);
            free(path);
            ctx->dir[ctx->idx].path = NULL;
            ctx->dir[ctx->idx].dir = NULL;
            if (ctx->idx) {
                ESP_LOGI(TAG, "continue with parent dir");
                ctx->idx--;
                path = ctx->dir[ctx->idx].path;
                dir = ctx->dir[ctx->idx].dir;
            } else {
                ESP_LOGI(TAG, "completed with the whole sdcard");
                audio_ctx->stop = true;
                break;
            }
        }
    }

    if (error || audio_ctx->stop) {
        ESP_LOGI(TAG, "clean up audio context data...");
        audio_ctx->stop = true;
        for (int i = ctx->idx; i >= 0; --i) {
            if (ctx->dir[i].path) {
                free(ctx->dir[i].path);
            }
            if (ctx->dir[i].dir) {
                closedir(ctx->dir[i].dir);
            }
        }
        free(ctx);
        audio_ctx->data = NULL;
    }

    return error;
}
