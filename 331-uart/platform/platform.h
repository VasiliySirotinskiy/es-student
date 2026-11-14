#pragma once

#include <stdbool.h>

// запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

void platform_led_set(bool on);

// нажата ли кнопка сейчас, без подавления дребезга
bool platform_button_read(void);

float platform_voltage_read(void);

// режим вывода GP22, который ведёт линию RC-цепи
typedef enum {
    PLATFORM_LINE_PUSH_PULL,
    PLATFORM_LINE_OPEN_DRAIN,
} platform_line_mode_t;

// единица или ноль на линии; в режиме открытого стока единица —
// отпущенная линия, которую поднимает внутренняя подтяжка
void platform_line_set(platform_line_mode_t mode, bool high);
