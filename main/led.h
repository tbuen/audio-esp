#pragma once

typedef enum {
    LED_RED,
    LED_GREEN,
    LED_YELLOW,
    LED_BLUE,
    LED_MAX
} led_color_t;

typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK
} led_mode_t;

void led_init(void);

void led_set(led_color_t color, led_mode_t mode);
