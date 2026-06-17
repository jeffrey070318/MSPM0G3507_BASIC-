#ifndef BSP_DWT_H
#define BSP_DWT_H

#include <stdint.h>

#include "ti_msp_dl_config.h"

static inline void DWT_Init(uint32_t cpu_mhz)
{
    (void) cpu_mhz;
}

static inline void DWT_Delay(float delay_s)
{
    if (delay_s <= 0.0f) {
        return;
    }

    delay_cycles((uint32_t) (CPUCLK_FREQ * delay_s));
}

static inline void DWT_Delayms(uint32_t delay_ms)
{
    delay_cycles((CPUCLK_FREQ / 1000U) * delay_ms);
}

static inline void DWT_Delayus(uint32_t delay_us)
{
    delay_cycles((CPUCLK_FREQ / 1000000U) * delay_us);
}

#endif
