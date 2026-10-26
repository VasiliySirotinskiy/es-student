#include "pi-task.h"

#include "pico/stdlib.h"

// По таблице курса: 585 тактов на член ряда при частоте 125 МГц.
// 80 * 585 / 125000000 = 0,0003744 с = 374,4 мкс < 0,5 мс.
#define PI_TERMS_PER_CALL 80

static pi_state_t pi_state = PI_STATE_NOT_STARTED;
static uint32_t pi_k = 0;
static uint32_t pi_n = 0;
static double pi_sum = 0.0;
static double pi_sign = 1.0;
static double pi_result = 0.0;
static uint64_t started_us = 0;
static uint64_t finished_us = 0;

void pi_task_init(void)
{
    pi_state = PI_STATE_NOT_STARTED;
    pi_k = 0;
    pi_n = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    pi_result = 0.0;
    started_us = 0;
    finished_us = 0;
}

bool pi_task_start(uint32_t terms)
{
    if (terms == 0 || pi_state == PI_STATE_RUNNING)
    {
        return false;
    }

    pi_n = terms;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    pi_result = 0.0;
    started_us = time_us_64();
    finished_us = 0;
    pi_state = PI_STATE_RUNNING;
    return true;
}

void pi_task_handle(void)
{
    if (pi_state != PI_STATE_RUNNING)
    {
        return;
    }

    uint32_t end = pi_k + PI_TERMS_PER_CALL;
    if (end < pi_k || end > pi_n)
    {
        end = pi_n;
    }

    for (; pi_k < end; pi_k++)
    {
        pi_sum += pi_sign / (2.0 * pi_k + 1.0);
        pi_sign = -pi_sign;
    }

    if (pi_k == pi_n)
    {
        pi_result = 4.0 * pi_sum;
        finished_us = time_us_64();
        pi_state = PI_STATE_READY;
    }
}

pi_state_t pi_task_get_state(void)
{
    return pi_state;
}

bool pi_task_get_result(double *result, uint64_t *elapsed_ms)
{
    if (pi_state != PI_STATE_READY)
    {
        return false;
    }

    *result = pi_result;
    *elapsed_ms = (finished_us - started_us) / 1000u;
    return true;
}
