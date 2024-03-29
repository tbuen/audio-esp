#include "errno.h"
#include "esp_log.h"
#include "fcntl.h"
#include "freertos/FreeRTOS.h"
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
#define MAX_DEPTH      5
#define MAX_CONTEXT    5

#define MIN(a,b) ((a) < (b) ? (a) : (b))

static const char *TAG = "audio";

typedef struct {
    char *path;
    DIR *dir;
} dir_t;

typedef struct {
    con_t con;
    // TODO time
    dir_t dir[MAX_DEPTH];
    uint8_t idx;
} context_t;

static TaskHandle_t handle;
static QueueHandle_t queue;
static QueueHandle_t request_queue;

static context_t context[MAX_CONTEXT];

static char *file2play;

static void audio_task(void *param);
static context_t *get_context(con_t con, bool start);
static void free_context(context_t *ctx);
static esp_err_t read_dir(con_t con, bool start, int16_t *error, audio_file_list_t *list);
static esp_err_t read_info(const char *filename, int16_t *error, audio_file_info_t *info);

void audio_init(QueueHandle_t q) {
    if (handle) return;

    queue = q;

    request_queue = xQueueCreate(10, sizeof(msg_audio_request_t));

    if (xTaskCreatePinnedToCore(&audio_task, "audio-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

void audio_request(const msg_audio_request_t *request) {
    xQueueSendToBack(request_queue, request, 0);
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
    msg_audio_request_t msg;

    vs_init();
    vs_set_volume(0x1414);

    for (;;) {
        if (xQueueReceive(request_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (res == ESP_FAIL) {
                res = vs_card_init(MOUNT_POINT);
            }

            switch (msg.request) {
                case AUDIO_REQ_FILE_LIST:
                    {
                        msg_audio_response_t *msg_data = calloc(1, sizeof(msg_audio_response_t));
                        msg_data->con = msg.con;
                        msg_data->rpc_id = msg.rpc_id;
                        if (res == ESP_OK) {
                            res = read_dir(msg.con, msg.start, &msg_data->error, &msg_data->list);
                        } else {
                            msg_data->error = AUDIO_IO_ERROR;
                        }
                        message_t msg = { BASE_AUDIO, EVENT_AUDIO_FILE_LIST, msg_data };
                        xQueueSendToBack(queue, &msg, 0);
                        break;
                    }
                case AUDIO_REQ_FILE_INFO:
                    {
                        msg_audio_response_t *msg_data = calloc(1, sizeof(msg_audio_response_t));
                        msg_data->con = msg.con;
                        msg_data->rpc_id = msg.rpc_id;
                        if (res == ESP_OK) {
                            res = read_info(msg.filename, &msg_data->error, &msg_data->info);
                        } else {
                            msg_data->error = AUDIO_IO_ERROR;
                        }
                        free(msg.filename);
                        message_t msg = { BASE_AUDIO, EVENT_AUDIO_FILE_INFO, msg_data };
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

static context_t *get_context(con_t con, bool start) {
    context_t *ctx = NULL;

    for (int i = 0; i < sizeof(context)/sizeof(context_t); ++i) {
        if (context[i].con == con) {
            ctx = &context[i];
            if (start) {
                free_context(ctx);
            }
            break;
        } else if (start && !ctx && !context[i].con) {
            ctx = &context[i];
        }
    }

    if (ctx) {
        ctx->con = con;
        // TODO ctx->time =
    }

    return ctx;
}

static void free_context(context_t *ctx) {
    ESP_LOGI(TAG, "clean up audio context data for con %d...", ctx->con);
    for (int i = ctx->idx; i >= 0; --i) {
        if (ctx->dir[i].path) {
            free(ctx->dir[i].path);
        }
        if (ctx->dir[i].dir) {
            closedir(ctx->dir[i].dir);
        }
    }
    memset(ctx, 0, sizeof(context_t));
}

static esp_err_t read_dir(con_t con, bool start, int16_t *error, audio_file_list_t *list) {
    struct dirent *entry;
    char *path = NULL;
    DIR *dir = NULL;

    context_t *ctx = get_context(con, start);

    if (ctx) {
        if (start) {
            list->first = true;
            asprintf(&ctx->dir[0].path, "%s", MOUNT_POINT);
            ctx->dir[0].dir = opendir(ctx->dir[0].path);
            if (!ctx->dir[0].dir) {
                ESP_LOGE(TAG, "error opening directory %s", ctx->dir[0].path);
                *error = AUDIO_IO_ERROR;
            }
        }
        path = ctx->dir[ctx->idx].path;
        dir = ctx->dir[ctx->idx].dir;
        ESP_LOGD(TAG, "start with idx %d and path %s", ctx->idx, ctx->dir[ctx->idx].path);
    } else {
        if (start) {
            *error = AUDIO_BUSY_ERROR;
        } else {
            *error = AUDIO_START_ERROR;
        }
    }

    while (*error == AUDIO_NO_ERROR && !list->last) {
        errno = 0;
        entry = readdir(dir);
        if (errno) {
            ESP_LOGE(TAG, "error reading directory");
            *error = AUDIO_IO_ERROR;
        } else if (entry) {
            if (entry->d_type == DT_REG) {
                ESP_LOGD(TAG, "File found: %s", entry->d_name);
                asprintf(&list->file[list->cnt].name, "%s/%s", &path[strlen(MOUNT_POINT)+1], entry->d_name);
                list->cnt++;
                if (list->cnt == FILE_LIST_SIZE) {
                    ESP_LOGD(TAG, "list full -> break");
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
                        *error = AUDIO_IO_ERROR;
                    }
                }
            }
        } else {
            ESP_LOGD(TAG, "no more files found in dir -> closing");
            closedir(dir);
            free(path);
            ctx->dir[ctx->idx].path = NULL;
            ctx->dir[ctx->idx].dir = NULL;
            if (ctx->idx) {
                ESP_LOGD(TAG, "continue with parent dir");
                ctx->idx--;
                path = ctx->dir[ctx->idx].path;
                dir = ctx->dir[ctx->idx].dir;
            } else {
                ESP_LOGD(TAG, "completed with the whole sdcard");
                list->last = true;
                break;
            }
        }
    }

    if (*error == AUDIO_IO_ERROR || list->last) {
        free_context(ctx);
    }

    if (*error == AUDIO_IO_ERROR) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

static esp_err_t read_info(const char *filename, int16_t *error, audio_file_info_t *info) {
    esp_err_t err = ESP_OK;
    char *file;
    if (strcmp(".ogg", &filename[strlen(filename) - 4])) {
        *error = AUDIO_FILE_TYPE_ERROR;
        return err;
    }
    asprintf(&file, "%s/%s", MOUNT_POINT, filename);
    errno = 0;
    int fd = open(file, O_RDONLY, 0);
    if (fd >= 0) {
        int n;
        uint8_t buffer[1024];
        uint64_t length = 0;
        uint32_t rate = 0;
        asprintf(&info->filename, "%s", filename);
        if ((n = read(fd, buffer, sizeof(buffer))) == sizeof(buffer)) {
            ESP_LOGD(TAG, "first buffer read");
            uint8_t *ptr = memmem(buffer, sizeof(buffer), "\x01vorbis", 7);
            if (ptr) {
                ESP_LOGD(TAG, "found 1st header");
                ptr += 7 + 5;
                rate = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
                ESP_LOGD(TAG, "rate: %d", rate);
            }
            ptr = memmem(buffer, sizeof(buffer), "\x03vorbis", 7);
            if (ptr) {
                ESP_LOGD(TAG, "found 2nd header");
                ptr += 7;
                uint32_t len = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
                ptr += 4 + len;
                uint32_t num = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
                ptr += 4;
                ESP_LOGD(TAG, "found %d comments", num);
                for (uint8_t c = 0; c < num; ++c) {
                    len = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
                    ptr += 4;
                    if (memcmp(ptr, "GENRE=", 6) == 0) {
                        info->genre = calloc(1, len - 6 + 1);
                        memcpy(info->genre, &ptr[6], len - 6);
                    } else if (memcmp(ptr, "ARTIST=", 7) == 0) {
                        info->artist = calloc(1, len - 7 + 1);
                        memcpy(info->artist, &ptr[7], len - 7);
                    } else if (memcmp(ptr, "ALBUM=", 6) == 0) {
                        info->album = calloc(1, len - 6 + 1);
                        memcpy(info->album, &ptr[6], len - 6);
                    } else if (memcmp(ptr, "TITLE=", 6) == 0) {
                        info->title = calloc(1, len - 6 + 1);
                        memcpy(info->title, &ptr[6], len - 6);
                    } else if (memcmp(ptr, "DATE=", 5) == 0) {
                        char buf[10] = { 0 };
                        memcpy(buf, &ptr[5], MIN(sizeof(buf) - 1, len - 5));
                        info->date = strtoul(buf, NULL, 10);
                    } else if (memcmp(ptr, "TRACKNUMBER=", 12) == 0) {
                        char buf[10] = { 0 };
                        memcpy(buf, &ptr[12], MIN(sizeof(buf) - 1, len - 12));
                        info->track = strtoul(buf, NULL, 10);
                    }
                    ptr += len;
                }
            }
        }
        lseek(fd, -sizeof(buffer), SEEK_END);
        while (true) {
            if ((n = read(fd, buffer, sizeof(buffer))) == sizeof(buffer)) {
                ESP_LOGD(TAG, "last buffer read");
                uint8_t *ptr = memmem(buffer, sizeof(buffer), "OggS\x00\x04", 6);
                if (ptr) {
                    ESP_LOGD(TAG, "found page header");
                    length = (uint64_t)ptr[6] + ((uint64_t)ptr[7] << 8) +
                        ((uint64_t)ptr[8] << 16) + ((uint64_t)ptr[9] << 24) +
                        ((uint64_t)ptr[10] << 32) + ((uint64_t)ptr[11] << 40) +
                        ((uint64_t)ptr[12] << 48) + ((uint64_t)ptr[13] << 56);
                    break;
                } else {
                    lseek(fd, - 2 * sizeof(buffer) + 32, SEEK_CUR);
                }
            } else {
                ESP_LOGW(TAG, "could not read, n: %d", n);
                break;
            }
        }
        if (length && rate) {
            info->duration = length / rate;
        }
        if (n < 0) {
            ESP_LOGE(TAG, "error reading file!");
            *error = AUDIO_IO_ERROR;
            err = ESP_FAIL;
        }
        close(fd);
    } else {
        // TODO only return ESP_FAIL in case of IO error.. in both cases errno is 2 :(
        ESP_LOGE(TAG, "error opening file %s: %d", file, errno);
        *error = AUDIO_FILE_NOT_FOUND_ERROR;
        err = ESP_FAIL;
    }
    free(file);
    return err;
}
