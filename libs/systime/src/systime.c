#include "systime/systime.h"

#include "pico/stdlib.h"

uint64_t systime_us(void)
{
    return time_us_64();
}

uint64_t systime_ms(void)
{
    return systime_us() / 1000u;
}

bool systime_period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = systime_us();

    if (now_us - *last_us < period_us)
    {
        return false;
    }

    // Следующее срабатывание отсчитывается от прошлого, а не от «сейчас»
    *last_us += period_us;

    // Отстали больше чем на период: не догонять, а начать отсчёт заново
    if (now_us - *last_us >= period_us)
    {
        *last_us = now_us;
    }

    return true;
}
