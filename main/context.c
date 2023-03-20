#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "string.h"

#include "config.h"
#include "message.h"
#include "context.h"

static const char *TAG = "context";

typedef struct {
    uint32_t uid;
    int sockfd;
} context_t;

static QueueHandle_t queue;
static SemaphoreHandle_t mutex;

static context_t context[MAX_CLIENT_CONNECTIONS];

static uint32_t next_uid = 1;

void context_init(QueueHandle_t q) {
    queue = q;
    mutex = xSemaphoreCreateMutex();
}

void context_create(int sockfd) {
    bool created = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) { 
        for (int i = 0; i < MAX_CLIENT_CONNECTIONS; ++i) {
            if (!context[i].uid) {
                context[i].uid = next_uid++;
                context[i].sockfd = sockfd;
                ESP_LOGI(TAG, "create uid %d socket %d", context[i].uid, context[i].sockfd);
                message_t msg = { BASE_CONTEXT, EVENT_CONTEXT_CREATED, NULL };
                xQueueSendToBack(queue, &msg, 0);
                created = true;
                break;
            }
        }
        xSemaphoreGive(mutex);
    }
    if (!created) {
        ESP_LOGE(TAG, "error creating context");
    }
}

void context_delete(int sockfd) {
    bool deleted = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) { 
        for (int i = 0; i < MAX_CLIENT_CONNECTIONS; ++i) {
            if (context[i].uid && context[i].sockfd == sockfd) {
                ESP_LOGI(TAG, "delete uid %d socket %d", context[i].uid, context[i].sockfd);
                memset(&context[i], 0, sizeof(context_t));
                message_t msg = { BASE_CONTEXT, EVENT_CONTEXT_DELETED, NULL };
                xQueueSendToBack(queue, &msg, 0);
                deleted = true;
                break;
            }
        }
        xSemaphoreGive(mutex);
    }
    if (!deleted) {
        ESP_LOGE(TAG, "error deleting context");
    }
}

size_t context_count(void) {
    size_t count = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) { 
        for (int i = 0; i < MAX_CLIENT_CONNECTIONS; ++i) {
            if (context[i].uid) {
                count++;
            }
        }
        xSemaphoreGive(mutex);
    }
    ESP_LOGI(TAG, "count %d", count);
    return count;
}
