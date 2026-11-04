// stream-task.h — задача потока: забирает у platform заполненные буферы
// отсчётов по порядку и отдаёт каждый кадром функции, переданной при запуске.
// Как кадр попадёт на компьютер, задача не знает.

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STREAM_STATE_IDLE,
    STREAM_STATE_RUN,
} stream_state_t;

// синхрослово: в памяти и в канале байты a5 5a
#define STREAM_SYNC_WORD 0x5aa5

// заголовок кадра по приложению А ТЗ: синхрослово, номер, число отсчётов;
// три поля по 16 бит, младший байт первым, без промежутков
typedef struct {
    uint16_t sync_word;
    uint16_t number;
    uint16_t count;
} stream_header_t;

// функция, которой задача отдаёт заголовок и данные каждого кадра
typedef void (*stream_task_callback_t)(const stream_header_t *header, const uint8_t *data,
                                       uint32_t bytes);

void stream_task_init(stream_task_callback_t on_frame);

// вызывается на каждой итерации суперцикла, не ждёт
void stream_task_handle(void);

// bits — 16 или 8; false — АЦП занят или разрядность не та
bool stream_task_start(uint8_t bits);
void stream_task_stop(void);

stream_state_t stream_task_get_state(void);
uint8_t stream_task_get_bits(void);

// итог потока с последнего запуска
uint32_t stream_task_get_frames(void);
uint32_t stream_task_get_samples(void);
uint32_t stream_task_get_overruns(void);
