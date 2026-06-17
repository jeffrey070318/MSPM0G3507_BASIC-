#include "bsp_dwt.h"

#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

static uint32_t cpu_freq_hz = CPUCLK_FREQ;

void DWT_Init(uint32_t CPU_Freq_mHz)
{
    if (CPU_Freq_mHz != 0U) {
        cpu_freq_hz = CPU_Freq_mHz * 1000000U;
    }
}

float DWT_GetDeltaT(uint32_t *cnt_last)
{
    uint32_t now = (uint32_t) DWT_GetTimeline_us();
    float dt = ((float) (now - *cnt_last)) / 1000000.0f;
    *cnt_last = now;

    return dt;
}

double DWT_GetDeltaT64(uint32_t *cnt_last)
{
    uint32_t now = (uint32_t) DWT_GetTimeline_us();
    double dt = ((double) (now - *cnt_last)) / 1000000.0;
    *cnt_last = now;

    return dt;
}

void DWT_SysTimeUpdate(void)
{
}

float DWT_GetTimeline_s(void)
{
    return DWT_GetTimeline_ms() * 0.001f;
}

float DWT_GetTimeline_ms(void)
{
#ifdef USE_FREERTOS
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        return (float) (xTaskGetTickCount() * portTICK_PERIOD_MS);
    }
#endif

    return 0.0f;
}

uint64_t DWT_GetTimeline_us(void)
{
    return (uint64_t) (DWT_GetTimeline_ms() * 1000.0f);
}

void DWT_Delay(float Delay)
{
    if (Delay <= 0.0f) {
        return;
    }

    delay_cycles((uint32_t) ((float) cpu_freq_hz * Delay));
}
