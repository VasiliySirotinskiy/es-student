#include "button-task/button-task.h"

#include <stddef.h>
#include "platform/platform.h"
#include "systime/systime.h"

typedef enum
{
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESS_DEBOUNCE,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASE_DEBOUNCE,
} button_state_t;

static const uint64_t DEBOUNCE_US = 20000;

static button_state_t button_state = BUTTON_STATE_RELEASED;
static uint64_t changed_us = 0;
static uint32_t press_count = 0;
static button_task_callback_t press_callback = NULL;

void button_task_init(button_task_callback_t on_press)
{
    press_callback = on_press;

    button_state = BUTTON_STATE_RELEASED;
    changed_us = 0;
    press_count = 0;
}

void button_task_handle(void)
{
    bool pressed = platform_button_read();
    uint64_t now_us = systime_us();

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
            if (press_callback != NULL)
            {
                press_callback();
            }
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
