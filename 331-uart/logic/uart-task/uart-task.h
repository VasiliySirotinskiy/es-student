// uart-task.h — задача обмена по UART: передаёт байты через один модуль,
// а когда передача кончилась и линия помолчала, отдаёт принятое другим модулем
// функции, переданной при запуске. Что делать с байтами, задача не знает.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/platform.h"

// самый длинный обмен: блок из 64 байтов
#define UART_TASK_MAX 64

// принятые байты и признаки ошибок приёма из UARTRSR
typedef void (*uart_task_callback_t)(const uint8_t *data, size_t size, uint32_t errors);

// false — обмен уже идёт или байтов больше UART_TASK_MAX
bool uart_task_start(platform_uart_t tx, platform_uart_t rx, const uint8_t *data, size_t size,
                     uart_task_callback_t on_done);

// вызывается на каждой итерации суперцикла, не ждёт
void uart_task_handle(void);
