#include "button-task.h"

#include "pico/stdlib.h"
#include "led-task/led-task.h"

typedef enum
{
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESS_DEBOUNCE,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASE_DEBOUNCE,
} button_state_t;

static const uint BUTTON_PIN = 15;
static const uint64_t DEBOUNCE_US = 20000;

static button_state_t button_state = BUTTON_STATE_RELEASED;
static uint64_t changed_us = 0;
static uint32_t press_count = 0;

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    button_state = BUTTON_STATE_RELEASED;
    changed_us = 0;
    press_count = 0;
}

void button_task_handle(void)
{
    bool pressed = !gpio_get(BUTTON_PIN);
    uint64_t now_us = time_us_64();

    switch (button_state)
    {
    case BUTTON_STATE_RELEASED:
        if (pressed)
        {
            changed_us = now_us;
            button_state = BUTTON_STATE_PRESS_DEBOUNCE;
        }
        break;

    case BUTTON_STATE_PRESS_DEBOUNCE:
        if (!pressed)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        else if (now_us - changed_us >= DEBOUNCE_US)
        {
            button_state = BUTTON_STATE_PRESSED;
            press_count++;
            led_task_next_state();
        }
        break;

    case BUTTON_STATE_PRESSED:
        if (!pressed)
        {
            changed_us = now_us;
            button_state = BUTTON_STATE_RELEASE_DEBOUNCE;
        }
        break;

    case BUTTON_STATE_RELEASE_DEBOUNCE:
        if (pressed)
        {
            button_state = BUTTON_STATE_PRESSED;
        }
        else if (now_us - changed_us >= DEBOUNCE_US)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        break;
    }
}

bool button_task_is_pressed(void)
{
    return button_state == BUTTON_STATE_PRESSED ||
           button_state == BUTTON_STATE_RELEASE_DEBOUNCE;
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}
