#include <driver/ledc.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "message.h"
#include "led.h"

// defines

#define TASK_CORE     1
#define TASK_PRIO     1
#define STACK_SIZE 4096

#define LED_MODE           LEDC_LOW_SPEED_MODE
#define LED_TIMER          LEDC_TIMER_0
#define LED_RESOLUTION     LEDC_TIMER_13_BIT
#define LED_FREQUENCY      (5000)
#define LED_DUTY_OFF       (0)
#define LED_DUTY_MAX       (8191)
#define LED_CHANNEL_RED    LEDC_CHANNEL_0
#define LED_CHANNEL_GREEN  LEDC_CHANNEL_1
#define LED_CHANNEL_YELLOW LEDC_CHANNEL_2
#define LED_CHANNEL_BLUE   LEDC_CHANNEL_3
#define LED_PIN_RED        GPIO_NUM_16
#define LED_PIN_GREEN      GPIO_NUM_17
#define LED_PIN_YELLOW     GPIO_NUM_18
#define LED_PIN_BLUE       GPIO_NUM_19

// types

typedef enum {
    LED_RED,
    LED_GREEN,
    LED_YELLOW,
    LED_BLUE
} led_color_t;

typedef enum {
    LED_OFF,
    LED_LOW,
    LED_HIGH,
} led_state_t;

typedef struct {
    ledc_channel_t ch;
    uint16_t duty[3];
} led_cfg_t;

// function prototypes

static void led_set(led_color_t color, led_state_t state);
static void led_task(void *param);

// local variables

static const char *TAG = "audio.led";
static TaskHandle_t handle;
static msg_handle_t msg_handle;
static led_cfg_t led_cfg[] = { { LED_CHANNEL_RED,    { 0, 200, 1600 } },
                               { LED_CHANNEL_GREEN,  { 0, 100,  800 } },
                               { LED_CHANNEL_YELLOW, { 0, 200, 1600 } },
                               { LED_CHANNEL_BLUE,   { 0, 300, 2400 } } };

// public functions

void led_init(void) {
    if (handle) return;

    msg_handle = msg_register(MSG_WLAN_STATUS|MSG_CON);

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

    if (xTaskCreatePinnedToCore(&led_task, "led-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

// local functions

static void led_set(led_color_t color, led_state_t state) {
    ESP_ERROR_CHECK(ledc_set_duty(LED_MODE, led_cfg[color].ch, led_cfg[color].duty[state]));
    ESP_ERROR_CHECK(ledc_update_duty(LED_MODE, led_cfg[color].ch));
}

static void led_task(void *param) {
    for (;;) {
        msg_t msg = msg_receive(msg_handle);
        if (msg.type == MSG_WLAN_STATUS)
        {
            switch (msg.value) {
                case WLAN_SCAN_STARTED:
                    led_set(LED_GREEN, LED_LOW);
                    break;
                case WLAN_SCAN_FINISHED:
                    led_set(LED_GREEN, LED_OFF);
                    break;
                case WLAN_CONNECTED:
                    led_set(LED_GREEN, LED_HIGH);
                    break;
                case WLAN_AP_STARTED:
                    led_set(LED_RED, LED_LOW);
                    break;
                case WLAN_AP_STOPPED:
                    led_set(LED_RED, LED_OFF);
                    break;
                case WLAN_AP_CONNECTED:
                    led_set(LED_RED, LED_HIGH);
                    break;
                case WLAN_AP_DISCONNECTED:
                    led_set(LED_RED, LED_LOW);
                    break;
                default:
                    break;
            }
        }
        if (msg.type == MSG_CON) {
            switch (msg.value) {
                case CON_CONNECTED:
                    if (con_count()) {
                        led_set(LED_YELLOW, LED_HIGH);
                    }
                    break;
                case CON_DISCONNECTED:
                    if (!con_count()) {
                        led_set(LED_YELLOW, LED_OFF);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}
