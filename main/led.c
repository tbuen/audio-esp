#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "led.h"

#define TASK_CORE     1
#define TASK_PRIO     1
#define STACK_SIZE 4096

#define LED_MODE           LEDC_LOW_SPEED_MODE
#define LED_TIMER          LEDC_TIMER_0
#define LED_RESOLUTION     LEDC_TIMER_13_BIT
#define LED_FREQUENCY      (5000)
#define LED_DUTY_OFF       (0)
#define LED_DUTY_ON        (8191)
#define LED_CHANNEL_RED    LEDC_CHANNEL_0
#define LED_CHANNEL_GREEN  LEDC_CHANNEL_1
#define LED_CHANNEL_YELLOW LEDC_CHANNEL_2
#define LED_CHANNEL_BLUE   LEDC_CHANNEL_3
#define LED_PIN_RED        GPIO_NUM_16
#define LED_PIN_GREEN      GPIO_NUM_17
#define LED_PIN_YELLOW     GPIO_NUM_18
#define LED_PIN_BLUE       GPIO_NUM_19

typedef struct {
    ledc_channel_t ch;
    led_mode_t mode;
    uint16_t duty;
} led_t;

static void led_task(void *param);

static const char *TAG = "led";

static QueueHandle_t queue;
static TaskHandle_t handle;
static led_t led[LED_MAX] = { { LED_CHANNEL_RED,    LED_OFF, LED_DUTY_ON * 20 / 100 },
                              { LED_CHANNEL_GREEN,  LED_OFF, LED_DUTY_ON * 10 / 100 },
                              { LED_CHANNEL_YELLOW, LED_OFF, LED_DUTY_ON * 20 / 100 },
                              { LED_CHANNEL_BLUE,   LED_OFF, LED_DUTY_ON * 30 / 100 } };

void led_init(void) {
    if (handle) return;

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LED_MODE,
        .timer_num        = LED_TIMER,
        .duty_resolution  = LED_RESOLUTION,
        .freq_hz          = LED_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel_red = {
        .speed_mode     = LED_MODE,
        .channel        = LED_CHANNEL_RED,
        .timer_sel      = LED_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PIN_RED,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_red));

    ledc_channel_config_t ledc_channel_green = {
        .speed_mode     = LED_MODE,
        .channel        = LED_CHANNEL_GREEN,
        .timer_sel      = LED_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PIN_GREEN,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_green));

    ledc_channel_config_t ledc_channel_yellow = {
        .speed_mode     = LED_MODE,
        .channel        = LED_CHANNEL_YELLOW,
        .timer_sel      = LED_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PIN_YELLOW,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_yellow));

    ledc_channel_config_t ledc_channel_blue = {
        .speed_mode     = LED_MODE,
        .channel        = LED_CHANNEL_BLUE,
        .timer_sel      = LED_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PIN_BLUE,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_blue));

    queue = xQueueCreate(5, sizeof(uint16_t));

    if (xTaskCreatePinnedToCore(&led_task, "led-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

void led_set(led_color_t color, led_mode_t mode) {
    uint16_t msg = (color << 8) | mode;
    xQueueSendToBack(queue, &msg, 0);
}

static void led_task(void *param) {
    uint16_t msg;
    uint8_t cnt = 0;

    for (;;) {
        if (xQueueReceive(queue, &msg, 0) == pdTRUE) {
            led_color_t color = msg >> 8;
            led_mode_t mode = msg & 0xFF;
            led[color].mode = mode;
        }

        for (uint8_t i = 0; i < LED_MAX; ++i) {
            if (   (led[i].mode == LED_ON)
                || (   (led[i].mode == LED_BLINK)
                    && (cnt > 28))) {
                ESP_ERROR_CHECK(ledc_set_duty(LED_MODE, led[i].ch, led[i].duty));
            } else {
                ESP_ERROR_CHECK(ledc_set_duty(LED_MODE, led[i].ch, LED_DUTY_OFF));
            }
            ESP_ERROR_CHECK(ledc_update_duty(LED_MODE, led[i].ch));
        }

        cnt = (cnt + 1) % 30;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
