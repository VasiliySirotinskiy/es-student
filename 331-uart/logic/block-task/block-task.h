// block-task.h — задача захвата блока: запускает захват блока отсчётов через DMA
// и, когда блок готов, отдаёт его функции, переданной при запуске.
// Что делать с отсчётами, задача не знает.

#pragma once

#include <stdbool.h>
#include <stdint.h>

// готовый блок: отсчёты, их число и время захвата
typedef void (*block_task_callback_t)(const uint16_t *samples, uint32_t count, uint32_t time_us);

// false — АЦП занят или блок больше, чем помещается в буфер
bool block_task_start(uint32_t count, block_task_callback_t on_block);

// вызывается на каждой итерации суперцикла, не ждёт
void block_task_handle(void);
