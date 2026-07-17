#include "bsp_timer.h"

#include "ti_msp_dl_config.h"

void TimerStart(void)
{
    DL_Timer_startCounter(TIMER_0_INST);
}

void TimerStop(void)
{
    DL_Timer_stopCounter(TIMER_0_INST);
}

void TimerReset(void)
{
    DL_Timer_stopCounter(TIMER_0_INST);
    /* TIMER_0 is configured in down-counting periodic mode. */
    DL_Timer_setTimerCount(TIMER_0_INST,
        DL_Timer_getLoadValue(TIMER_0_INST));
}

uint32_t TimerRead(void)
{
    return DL_Timer_getTimerCount(TIMER_0_INST);
}

uint32_t TimerGetClockFreq(void)
{
    return TIMER_0_CLOCK_HZ;
}

uint32_t TimerGetPeriod(void)
{
    return DL_Timer_getLoadValue(TIMER_0_INST) + 1U;
}
