#include "button-task/button-task.h"

#include <stddef.h>
#include "systime/systime.h"

typedef enum
{
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESS_DEBOUNCE,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASE_DEBOUNCE,
} button_state_t;

typedef struct
{
    button_state_t state;
    uint64_t changed_us;
    uint64_t pressed_us;
    uint32_t press_count;
    button_task_callback_t on_short;
    button_task_callback_t on_long;
    bool long_reported;
} button_context_t;

static const uint64_t DEBOUNCE_US = 20000;
static const uint64_t LONG_PRESS_US = 1000000;
static button_context_t contexts[PLATFORM_BUTTON_COUNT];

void button_task_init(platform_button_t button, button_task_callback_t on_short,
                      button_task_callback_t on_long)
{
    if ((uint32_t)button >= PLATFORM_BUTTON_COUNT)
    {
        return;
    }

    contexts[button].state = BUTTON_STATE_RELEASED;
    contexts[button].changed_us = 0;
    contexts[button].pressed_us = 0;
    contexts[button].press_count = 0;
    contexts[button].on_short = on_short;
    contexts[button].on_long = on_long;
    contexts[button].long_reported = false;
}

void button_task_handle(void)
{
    uint64_t now_us = systime_us();

    for (uint32_t index = 0; index < PLATFORM_BUTTON_COUNT; index++)
    {
        platform_button_t button = (platform_button_t)index;
        button_context_t *context = &contexts[index];
        bool pressed = platform_button_read(button);

        switch (context->state)
        {
        case BUTTON_STATE_RELEASED:
            if (pressed)
            {
                context->changed_us = now_us;
                context->state = BUTTON_STATE_PRESS_DEBOUNCE;
            }
            break;

        case BUTTON_STATE_PRESS_DEBOUNCE:
            if (!pressed)
            {
                context->state = BUTTON_STATE_RELEASED;
            }
            else if (now_us - context->changed_us >= DEBOUNCE_US)
            {
                context->state = BUTTON_STATE_PRESSED;
                context->press_count++;
                context->pressed_us = now_us;
                context->long_reported = false;
                if (context->on_long == NULL && context->on_short != NULL)
                {
                    context->on_short();
                }
            }
            break;

        case BUTTON_STATE_PRESSED:
            if (!pressed)
            {
                context->changed_us = now_us;
                context->state = BUTTON_STATE_RELEASE_DEBOUNCE;
            }
            else if (context->on_long != NULL && !context->long_reported &&
                     now_us - context->pressed_us >= LONG_PRESS_US)
            {
                context->long_reported = true;
                context->on_long();
            }
            break;

        case BUTTON_STATE_RELEASE_DEBOUNCE:
            if (pressed)
            {
                context->state = BUTTON_STATE_PRESSED;
            }
            else if (now_us - context->changed_us >= DEBOUNCE_US)
            {
                context->state = BUTTON_STATE_RELEASED;
                if (context->on_long != NULL && !context->long_reported &&
                    context->on_short != NULL)
                {
                    context->on_short();
                }
            }
            break;
        }
    }
}

bool button_task_is_pressed(platform_button_t button)
{
    if ((uint32_t)button >= PLATFORM_BUTTON_COUNT)
    {
        return false;
    }

    return contexts[button].state == BUTTON_STATE_PRESSED ||
           contexts[button].state == BUTTON_STATE_RELEASE_DEBOUNCE;
}

uint32_t button_task_get_press_count(platform_button_t button)
{
    if ((uint32_t)button >= PLATFORM_BUTTON_COUNT)
    {
        return 0;
    }

    return contexts[button].press_count;
}
