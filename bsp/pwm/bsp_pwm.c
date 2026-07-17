#include "bsp_pwm.h"

#include <stdlib.h>
#include <string.h>

static uint8_t idx;
static PWMInstance *pwm_instance[PWM_DEVICE_CNT] = {NULL};

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

PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    if ((config == NULL) || (config->htim == NULL) ||
        (config->htim->Instance == NULL) ||
        (idx >= PWM_DEVICE_CNT)) {
        return NULL;
    }

    PWMInstance *pwm = (PWMInstance *) malloc(sizeof(PWMInstance));
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
        free(pwm);
        return NULL;
    }

    PWMSetPeriod(pwm, pwm->period);
    PWMSetDutyRatio(pwm, pwm->dutyratio);
    PWMStart(pwm);

    pwm_instance[idx++] = pwm;
    return pwm;
}

void PWMStart(PWMInstance *pwm)
{
    if ((pwm != NULL) && (pwm->htim != NULL) &&
        (pwm->htim->Instance != NULL)) {
        DL_Timer_startCounter(pwm->htim->Instance);
    }
}

void PWMStop(PWMInstance *pwm)
{
    if ((pwm != NULL) && (pwm->htim != NULL) &&
        (pwm->htim->Instance != NULL)) {
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
    DL_Timer_setLoadValue(pwm->htim->Instance, pwm->htim->period_ticks);
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
    DL_Timer_setCaptureCompareValue(
        pwm->htim->Instance, compare, (DL_TIMER_CC_INDEX) pwm->channel);
    pwm->dutyratio = dutyratio;
}

void PWMStartDMA(PWMInstance *pwm, uint32_t *pData, uint32_t Size)
{
    (void) pData;
    (void) Size;

    /* No PWM DMA channel is currently owned by SysConfig. */
    PWMStart(pwm);
}
