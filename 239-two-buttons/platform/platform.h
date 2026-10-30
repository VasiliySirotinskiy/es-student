#pragma once

#include <stdbool.h>

typedef enum
{
    PLATFORM_BUTTON_1,
    PLATFORM_BUTTON_2,
    PLATFORM_BUTTON_COUNT,
} platform_button_t;

// запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

void platform_led_set(bool on);

// нажата ли кнопка сейчас, без подавления дребезга
bool platform_button_read(platform_button_t button);
