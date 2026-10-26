#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PROFILING_STOPWATCH_COUNT 8

// Суммы храните в 32 битах. Это дешевле, и сумма замеров одного секундомера
// переполнится только через 71 минуту.
typedef struct
{
    const char *name;
    uint32_t count;
    float mean_us;
    uint32_t max_us;
} profiling_result_t;

void profiling_stopwatch_init(uint32_t id, const char *name);

void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);

// false — секундомер с таким номером не заведён
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);
