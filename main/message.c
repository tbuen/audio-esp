#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "message.h"

#define MAX_HANDLES     20
#define MAX_MESSAGES    10

typedef struct {
    uint32_t types;
    QueueHandle_t queue;
} receiver_t;

static receiver_t receiver[MAX_HANDLES];
static msg_handle_t next_handle;
static SemaphoreHandle_t mutex;

void msg_init(void) {
    mutex = xSemaphoreCreateMutex();
}

msg_handle_t msg_register(uint8_t msg_types) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    assert(next_handle < MAX_HANDLES);
    msg_handle_t handle = next_handle++;
    receiver[handle].types = msg_types;
    receiver[handle].queue = xQueueCreate(MAX_MESSAGES, sizeof(msg_t));
    xSemaphoreGive(mutex);
    return handle;
}

void msg_send_value(uint8_t msg_type, uint8_t value) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    msg_t msg = {
        .type = msg_type,
        .value = value
    };
    for (int i = 0; i < next_handle; ++i) {
        if (receiver[i].types & msg_type) {
            assert(xQueueSendToBack(receiver[i].queue, &msg, 0) == pdPASS);
        }
    }
    xSemaphoreGive(mutex);
}

msg_t msg_receive(msg_handle_t handle) {
    assert(handle < MAX_HANDLES);
    msg_t msg;
    assert(xQueueReceive(receiver[handle].queue, &msg, portMAX_DELAY) == pdPASS);
    return msg;
}
