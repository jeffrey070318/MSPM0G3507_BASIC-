#include "bsp_pwm.h"
#include "bsp_memory.h"

#include <string.h>

#include "ti_msp_dl_config.h"

static uint8_t idx;
static PWMInstance *pwm_instance[PWM_DEVICE_CNT] = {NULL};

/* TIMA0 is at 0x400C0000 on MSPM0G3507. */
#define TIMA0_BASE_ADDR  ((uint32_t)0x400C0000U)

static bool PWMIsTimerA(TIM_HandleTypeDef *htim)
{
    if (htim == NULL) {
        return false;
    }
    uint32_t addr = (uint32_t)(htim->Instance);
    return (addr >= TIMA0_BASE_ADDR) &&
           (addr < (TIMA0_BASE_ADDR + 0x2000U));
}

static uint32_t PWMSelectClock(TIM_HandleTypeDef *htim)
{
    if ((htim != NULL) && (htim->tclk_hz != 0U)) {
        return htim->tclk_hz;
    }
    return 0U;
}

static uint32_t PWMPeriodToTicks(PWMInstance *pwm, float period)
{
    if ((period <= 0.0f) || (pwm == NULL) ||
        (pwm->tclk == 0U)) {
        return 1U;
    }

    float ticks = period * (float) pwm->tclk;
    if (ticks < 1.0f) {
        return 1U;
    }
    if (ticks > 4294967295.0f) {
        return UINT32_MAX;
    }
    return (uint32_t) ticks;
}

static void PWMSetLoadValue(TIM_HandleTypeDef *htim, uint32_t ticks)
{
    if (PWMIsTimerA(htim)) {
        DL_TimerA_setLoadValue(
            (GPTIMER_Regs *)(htim->Instance), ticks);
    } else {
        DL_Timer_setLoadValue(htim->Instance, ticks);
    }
}

static void PWMSetCCValue(
    TIM_HandleTypeDef *htim, uint32_t value, uint32_t channel)
{
    if (PWMIsTimerA(htim)) {
        DL_TimerA_setCaptureCompareValue(
            (GPTIMER_Regs *)(htim->Instance),
            value, (DL_TIMER_CC_INDEX)channel);
    } else {
        DL_Timer_setCaptureCompareValue(
            htim->Instance, value, (DL_TIMER_CC_INDEX)channel);
    }
}

PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    if ((config == NULL) || (config->htim == NULL) ||
        (config->htim->Instance == NULL) ||
        (idx >= PWM_DEVICE_CNT)) {
        return NULL;
    }

    PWMInstance *pwm = (PWMInstance *) BSPMalloc(sizeof(PWMInstance));
    if (pwm == NULL) {
        return NULL;
    }
    memset(pwm, 0, sizeof(PWMInstance));

    pwm->htim = config->htim;
    pwm->channel = config->channel;
    pwm->tclk = PWMSelectClock(config->htim);
    pwm->period = config->period;
    pwm->dutyratio = config->dutyratio;
    pwm->callback = config->callback;
    pwm->id = config->id;

    if (pwm->tclk == 0U) {
        BSPFree(pwm);
        return NULL;
    }

    PWMSetLoadValue(pwm->htim, PWMPeriodToTicks(pwm, pwm->period));
    PWMSetCCValue(pwm->htim, 0U, pwm->channel);
    PWMStart(pwm);

    pwm_instance[idx++] = pwm;
    return pwm;
}

void PWMStart(PWMInstance *pwm)
{
    if ((pwm == NULL) || (pwm->htim == NULL) ||
        (pwm->htim->Instance == NULL)) {
        return;
    }

    if (PWMIsTimerA(pwm->htim)) {
        DL_TimerA_startCounter(
            (GPTIMER_Regs *)(pwm->htim->Instance));
    } else {
        DL_Timer_startCounter(pwm->htim->Instance);
    }
}

void PWMStop(PWMInstance *pwm)
{
    if ((pwm == NULL) || (pwm->htim == NULL) ||
        (pwm->htim->Instance == NULL)) {
        return;
    }

    if (PWMIsTimerA(pwm->htim)) {
        DL_TimerA_stopCounter(
            (GPTIMER_Regs *)(pwm->htim->Instance));
    } else {
        DL_Timer_stopCounter(pwm->htim->Instance);
    }
}

void PWMSetPeriod(PWMInstance *pwm, float period)
{
    if ((pwm == NULL) || (pwm->htim == NULL) ||
        (pwm->htim->Instance == NULL)) {
        return;
    }

    pwm->htim->period_ticks = PWMPeriodToTicks(pwm, period);
    pwm->period = period;
    PWMSetLoadValue(pwm->htim, pwm->htim->period_ticks);
}

void PWMSetDutyRatio(PWMInstance *pwm, float dutyratio)
{
    if ((pwm == NULL) || (pwm->htim == NULL) ||
        (pwm->htim->Instance == NULL)) {
        return;
    }

    if (dutyratio < 0.0f) {
        dutyratio = 0.0f;
    } else if (dutyratio > 1.0f) {
        dutyratio = 1.0f;
    }

    uint32_t compare =
        (uint32_t) ((float) pwm->htim->period_ticks * dutyratio);
    PWMSetCCValue(pwm->htim, compare, pwm->channel);
    pwm->dutyratio = dutyratio;
}

void PWMStartDMA(PWMInstance *pwm, uint32_t *pData, uint32_t Size)
{
    (void) pData;
    (void) Size;
    PWMStart(pwm);
}