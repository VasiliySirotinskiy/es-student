#include "profiling/profiling.h"

#include <stddef.h>
#include "systime/systime.h"

typedef struct
{
    const char *name;
    uint32_t started_us;
    uint32_t sum_us;
    uint32_t count;
    uint32_t max_us;
    bool initialized;
    bool running;
} stopwatch_t;

static stopwatch_t stopwatches[PROFILING_STOPWATCH_COUNT];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id >= PROFILING_STOPWATCH_COUNT || name == NULL)
    {
        return;
    }

    stopwatches[id] = (stopwatch_t){.name = name, .initialized = true};
}

#if PROFILING_ENABLED
void profiling_start(uint32_t id)
{
    if (id >= PROFILING_STOPWATCH_COUNT || !stopwatches[id].initialized)
    {
        return;
    }

    stopwatches[id].started_us = (uint32_t)systime_us();
    stopwatches[id].running = true;
}

void profiling_stop(uint32_t id)
{
    if (id >= PROFILING_STOPWATCH_COUNT || !stopwatches[id].initialized ||
        !stopwatches[id].running)
    {
        return;
    }

    stopwatch_t *stopwatch = &stopwatches[id];
    uint32_t elapsed_us = (uint32_t)systime_us() - stopwatch->started_us;
    stopwatch->running = false;
    stopwatch->sum_us += elapsed_us;
    stopwatch->count++;

    if (elapsed_us > stopwatch->max_us)
    {
        stopwatch->max_us = elapsed_us;
    }
}
#endif

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_STOPWATCH_COUNT || !stopwatches[id].initialized ||
        result == NULL)
    {
        return false;
    }

    const stopwatch_t *stopwatch = &stopwatches[id];
    *result = (profiling_result_t){
        .name = stopwatch->name,
        .count = stopwatch->count,
        .mean_us = stopwatch->count == 0
                       ? 0.0f
                       : (float)stopwatch->sum_us / (float)stopwatch->count,
        .max_us = stopwatch->max_us,
    };
    return true;
}

void profiling_reset(void)
{
    uint32_t now_us = (uint32_t)systime_us();

    for (uint32_t id = 0; id < PROFILING_STOPWATCH_COUNT; id++)
    {
        stopwatch_t *stopwatch = &stopwatches[id];
        if (!stopwatch->initialized)
        {
            continue;
        }

        stopwatch->started_us = stopwatch->running ? now_us : 0;
        stopwatch->sum_us = 0;
        stopwatch->count = 0;
        stopwatch->max_us = 0;
    }
}
