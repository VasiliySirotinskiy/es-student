#include "led-task/led-task.h"

#include "platform/platform.h"
#include "systime/systime.h"

static led_state_t led_state = LED_STATE_OFF;
static bool led_on = false;
static uint32_t blink_period_ms = 1000;
static uint64_t half_period_us = 500000;
static uint64_t last_toggle_us = 0;

void led_task_init(void)
{
    led_on = false;
    platform_led_set(led_on);
    led_task_set_period_ms(1000);
    led_task_set_state(LED_STATE_BLINK);
}

void led_task_set_state(led_state_t state)
{
    led_state = state;

    switch (led_state)
    {
    case LED_STATE_OFF:
        led_on = false;
        platform_led_set(led_on);
        break;
    case LED_STATE_ON:
        led_on = true;
        platform_led_set(led_on);
        break;
    case LED_STATE_BLINK:
        last_toggle_us = systime_us() - half_period_us;
        break;
    }
}

void led_task_handle(void)
{
    switch (led_state)
    {
    case LED_STATE_OFF:
    case LED_STATE_ON:
        break;
    case LED_STATE_BLINK:
        if (systime_period_elapsed(&last_toggle_us, half_period_us))
        {
            led_on = !led_on;
            platform_led_set(led_on);
        }
        break;
    }
}

led_state_t led_task_get_state(void)
{
    return led_state;
}

void led_task_next_state(void)
{
    switch (led_state)
    {
    case LED_STATE_OFF:
        led_task_set_state(LED_STATE_ON);
        break;
    case LED_STATE_ON:
        led_task_set_state(LED_STATE_BLINK);
        break;
    case LED_STATE_BLINK:
        led_task_set_state(LED_STATE_OFF);
        break;
    }
}

bool led_task_set_period_ms(uint32_t period_ms)
{
    if (period_ms == 0)
    {
        return false;
    }

    blink_period_ms = period_ms;
    half_period_us = (uint64_t)period_ms * 1000u / 2u;
    return true;
}

uint32_t led_task_get_period_ms(void)
{
    return blink_period_ms;
}
