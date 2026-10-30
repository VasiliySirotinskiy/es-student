#include "platform/platform.h"

#include "pico/stdlib.h"

static const uint LED_PIN = 25;
static const uint BUTTON_PINS[PLATFORM_BUTTON_COUNT] = { 15, 14 };

void platform_init(void)
{
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, false);
    for (uint button = 0; button < PLATFORM_BUTTON_COUNT; button++)
    {
        gpio_init(BUTTON_PINS[button]);
        gpio_set_dir(BUTTON_PINS[button], GPIO_IN);
        gpio_pull_up(BUTTON_PINS[button]);
    }
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_button_read(platform_button_t button)
{
    if ((unsigned)button >= PLATFORM_BUTTON_COUNT)
    {
        return false;
    }

    return !gpio_get(BUTTON_PINS[button]);
}
