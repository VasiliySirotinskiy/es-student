#include "edge-task/edge-task.h"

#include <stddef.h>
#include "systime/systime.h"

typedef enum
{
    EDGE_STATE_IDLE,
    EDGE_STATE_DISCHARGE,
    EDGE_STATE_RISE,
    EDGE_STATE_CHARGE,
    EDGE_STATE_FALL,
} edge_state_t;

static const float EMPTY_V = 0.05f;
static const float RISE_V = 2.0f;
static const float FULL_V = 3.2f;
static const float FALL_V = 1.3f;
static const uint64_t TIMEOUT_US = 2000000;

static edge_state_t edge_state = EDGE_STATE_IDLE;
static platform_line_mode_t line_mode = PLATFORM_LINE_PUSH_PULL;
static edge_task_callback_t result_callback = NULL;
static uint64_t phase_started_us = 0;
static uint32_t rise_us = 0;

static void enter(edge_state_t next_state, bool high)
{
    platform_line_set(line_mode, high);
    phase_started_us = systime_us();
    edge_state = next_state;
}

static void finish(bool ok, uint32_t fall_us)
{
    edge_task_callback_t callback = result_callback;
    platform_line_mode_t completed_mode = line_mode;
    uint32_t completed_rise_us = rise_us;

    edge_state = EDGE_STATE_IDLE;
    result_callback = NULL;
    if (callback != NULL)
    {
        callback(completed_mode, ok, completed_rise_us, fall_us);
    }
}

bool edge_task_start(platform_line_mode_t mode, edge_task_callback_t on_done)
{
    if (edge_state != EDGE_STATE_IDLE)
    {
        return false;
    }

    line_mode = mode;
    result_callback = on_done;
    rise_us = 0;
    enter(EDGE_STATE_DISCHARGE, false);
    return true;
}

void edge_task_handle(void)
{
    if (edge_state == EDGE_STATE_IDLE)
    {
        return;
    }

    uint64_t elapsed_us = systime_us() - phase_started_us;
    float voltage = platform_voltage_read();
    if (elapsed_us > TIMEOUT_US)
    {
        finish(false, 0);
        return;
    }

    switch (edge_state)
    {
    case EDGE_STATE_IDLE:
        break;

    case EDGE_STATE_DISCHARGE:
        if (voltage < EMPTY_V)
        {
            enter(EDGE_STATE_RISE, true);
        }
        break;

    case EDGE_STATE_RISE:
        if (voltage > RISE_V)
        {
            rise_us = (uint32_t)elapsed_us;
            enter(EDGE_STATE_CHARGE, true);
        }
        break;

    case EDGE_STATE_CHARGE:
        if (voltage > FULL_V)
        {
            enter(EDGE_STATE_FALL, false);
        }
        break;

    case EDGE_STATE_FALL:
        if (voltage < FALL_V)
        {
            finish(true, (uint32_t)elapsed_us);
        }
        break;
    }
}
