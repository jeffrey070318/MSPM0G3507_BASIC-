#include "bsp_capture.h"

#include "ti_msp_dl_config.h"

static GPTIMER_Regs *CaptureGetInstance(Capture_Device_e capture)
{
    switch (capture) {
#ifdef CAPTURE_0_INST
    case CAPTURE_DEVICE_0:
        return CAPTURE_0_INST;
#endif
#ifdef CAPTURE_1_INST
    case CAPTURE_DEVICE_1:
        return CAPTURE_1_INST;
#endif
    default:
        return NULL;
    }
}

static uint32_t CaptureGetConfiguredClock(Capture_Device_e capture)
{
    switch (capture) {
#ifdef CAPTURE_0_INST
    case CAPTURE_DEVICE_0:
        return 80000000U;
#endif
#ifdef CAPTURE_1_INST
    case CAPTURE_DEVICE_1:
        return 40000000U;
#endif
    default:
        return 0U;
    }
}

static uint32_t CaptureGetConfiguredPeriod(Capture_Device_e capture)
{
    switch (capture) {
#ifdef CAPTURE_0_INST_LOAD_VALUE
    case CAPTURE_DEVICE_0:
        return CAPTURE_0_INST_LOAD_VALUE + 1U;
#endif
#ifdef CAPTURE_1_INST_LOAD_VALUE
    case CAPTURE_DEVICE_1:
        return CAPTURE_1_INST_LOAD_VALUE + 1U;
#endif
    default:
        return 0U;
    }
}

void CaptureStart(Capture_Device_e capture)
{
    GPTIMER_Regs *timer = CaptureGetInstance(capture);
    if (timer == NULL) {
        return;
    }

    DL_Timer_startCounter(timer);
}

void CaptureStop(Capture_Device_e capture)
{
    GPTIMER_Regs *timer = CaptureGetInstance(capture);
    if (timer == NULL) {
        return;
    }

    DL_Timer_stopCounter(timer);
}

void CaptureReset(Capture_Device_e capture)
{
    GPTIMER_Regs *timer = CaptureGetInstance(capture);
    if (timer == NULL) {
        return;
    }

    DL_Timer_stopCounter(timer);
    DL_Timer_setTimerCount(timer, DL_Timer_getLoadValue(timer));
}

uint32_t CaptureRead(Capture_Device_e capture)
{
    GPTIMER_Regs *timer = CaptureGetInstance(capture);
    if (timer == NULL) {
        return 0U;
    }

    return DL_Timer_getCaptureCompareValue(timer, DL_TIMER_CC_0_INDEX);
}

uint32_t CaptureReadTimeUs(Capture_Device_e capture)
{
    uint32_t captured_ticks = CaptureRead(capture);
    uint32_t clock_hz = CaptureGetConfiguredClock(capture);
    uint32_t period_ticks = CaptureGetConfiguredPeriod(capture);

    if ((clock_hz == 0U) || (period_ticks == 0U) ||
        (captured_ticks > period_ticks)) {
        return 0U;
    }

    /* Edge-time capture is configured as a down counter. */
    return (uint32_t) ((((uint64_t) (period_ticks - captured_ticks) *
                         1000000ULL) +
                        (clock_hz / 2U)) /
                       clock_hz);
}

uint32_t CaptureGetClockFreq(Capture_Device_e capture)
{
    return CaptureGetConfiguredClock(capture);
}

uint32_t CaptureGetPeriod(Capture_Device_e capture)
{
    return CaptureGetConfiguredPeriod(capture);
}
