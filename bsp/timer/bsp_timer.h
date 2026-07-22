#ifndef BSP_TIMER_H
#define BSP_TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TIMER_0 is configured in SysConfig as a 40 MHz, 1 ms periodic timer. */
#define TIMER_0_CLOCK_HZ (40000000U)

void TimerStart(void);
void TimerStop(void);
void TimerReset(void);

/** Read the current TIMER_0 counter value in timer ticks. */
uint32_t TimerRead(void);

/** Return the configured timer input clock in Hz. */
uint32_t TimerGetClockFreq(void);

/** Return the configured timer period in ticks, including the terminal count. */
uint32_t TimerGetPeriod(void);

#ifdef __cplusplus
}
#endif

#endif
