#include "bsp_capture.h"

#include "ti_msp_dl_config.h"

static GPTIMER_Regs *CaptureGetInstance(Capture_Device_e capture)
{
    switch (capture) {
    case CAPTURE_DEVICE_0:
        return CAPTURE_0_INST;
    case CAPTURE_DEVICE_1:
        return CAPTURE_1_INST;
    default:
        return NULL;
    }
}

static uint32_t CaptureGetConfiguredClock(Capture_Device_e capture)
{
    switch (capture) {
    case CAPTURE_DEVICE_0:
        return 80000000U;
    case CAPTURE_DEVICE_1:
        return 40000000U;
    default:
        return 0U;
    }
}

static uint32_t CaptureGetConfiguredPeriod(Capture_Device_e capture)
{
    switch (capture) {
    case CAPTURE_DEVICE_0:
        return CAPTURE_0_INST_LOAD_VALUE + 1U;
    case CAPTURE_DEVICE_1:
        return CAPTURE_1_INST_LOAD_VALUE + 1U;
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

uint32_t CaptureReadCount(Capture_Device_e capture)
{
    GPTIMER_Regs *timer = CaptureGetInstance(capture);
    if (timer == NULL) {
        return 0U;
    }

    return DL_Timer_getTimerCount(timer);
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


/* ======== Encoder tracking ======== */

#define CAPTURE_ENCODER_COUNT 2U

typedef struct {
    int32_t  total;
    int16_t  delta;
    uint32_t last_val;
    bool     active;
} CaptureEncoderState_t;

static CaptureEncoderState_t g_enc[CAPTURE_ENCODER_COUNT];

void CaptureEncoderInit(Capture_Device_e capture)
{
    if ((uint32_t)capture >= CAPTURE_ENCODER_COUNT) {
        return;
    }

    g_enc[capture].total    = 0;
    g_enc[capture].delta    = 0;
    g_enc[capture].last_val = 0U;
    g_enc[capture].active   = true;

    CaptureStart(capture);

    /* Prime last_val with current count so first delta is zero. */
    if (capture == CAPTURE_DEVICE_0) {
        g_enc[capture].last_val = CaptureReadCount(capture);
    } else {
        g_enc[capture].last_val = CaptureRead(capture);
    }
}

void CaptureEncoderUpdate(Capture_Device_e capture)
{
    if ((uint32_t)capture >= CAPTURE_ENCODER_COUNT || !g_enc[capture].active) {
        return;
    }

    uint32_t period = CaptureGetPeriod(capture);
    int16_t  delta  = 0;

    if (capture == CAPTURE_DEVICE_0) {
        /* Hardware EDGE_COUNT: read timer counter directly. */
        uint32_t raw = CaptureReadCount(capture);
        int32_t diff = (int32_t)(raw - g_enc[capture].last_val);
        if (diff < 0) {
            diff += (int32_t)period;
        }
        delta = (int16_t)diff;
        g_enc[capture].last_val = raw;
    } else {
        /* Software EDGE_TIME: detect capture-value change = one edge. */
        uint32_t raw = CaptureRead(capture);
        if (raw != g_enc[capture].last_val) {
            delta = 1;
            g_enc[capture].last_val = raw;
        }
    }

    g_enc[capture].delta  = delta;
    g_enc[capture].total += delta;
}

int32_t CaptureEncoderGetTotal(Capture_Device_e capture)
{
    if ((uint32_t)capture >= CAPTURE_ENCODER_COUNT) {
        return 0;
    }
    return g_enc[capture].total;
}

int16_t CaptureEncoderGetDelta(Capture_Device_e capture)
{
    if ((uint32_t)capture >= CAPTURE_ENCODER_COUNT) {
        return 0;
    }
    return g_enc[capture].delta;
}
