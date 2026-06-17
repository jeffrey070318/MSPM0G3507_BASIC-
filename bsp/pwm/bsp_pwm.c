#include "bsp_pwm.h"

#include "memory.h"
#include "stdlib.h"

static uint8_t idx;
static PWMInstance *pwm_instance[PWM_DEVICE_CNT] = {NULL};

static uint32_t PWMSelectTclk(TIM_HandleTypeDef *htim);
static uint32_t PWMPeriodToTicks(PWMInstance *pwm, float period);

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    for (uint8_t i = 0; i < idx; i++) {
        if ((pwm_instance[i]->htim == htim) &&
            (htim->Channel == pwm_instance[i]->channel)) {
            if (pwm_instance[i]->callback) {
                pwm_instance[i]->callback(pwm_instance[i]);
            }
            return;
        }
    }
}

PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    if (idx >= PWM_DEVICE_CNT) {
        while (1) {
        }
    }

    PWMInstance *pwm = (PWMInstance *) malloc(sizeof(PWMInstance));
    memset(pwm, 0, sizeof(PWMInstance));

    pwm->htim = config->htim;
    pwm->channel = config->channel;
    pwm->period = config->period;
    pwm->dutyratio = config->dutyratio;
    pwm->callback = config->callback;
    pwm->id = config->id;
    pwm->tclk = PWMSelectTclk(pwm->htim);

    PWMSetPeriod(pwm, pwm->period);
    PWMSetDutyRatio(pwm, pwm->dutyratio);
    PWMStart(pwm);

    pwm_instance[idx++] = pwm;
    return pwm;
}

void PWMStart(PWMInstance *pwm)
{
    if ((pwm == NULL) || (pwm->htim == NULL) || (pwm->htim->Instance == NULL)) {
        return;
    }

    DL_Timer_startCounter(pwm->htim->Instance);
}

void PWMStop(PWMInstance *pwm)
{
    if ((pwm == NULL) || (pwm->htim == NULL) || (pwm->htim->Instance == NULL)) {
        return;
    }

    DL_Timer_stopCounter(pwm->htim->Instance);
}

void PWMSetPeriod(PWMInstance *pwm, float period)
{
    if ((pwm == NULL) || (pwm->htim == NULL) || (pwm->htim->Instance == NULL)) {
        return;
    }

    uint32_t ticks = PWMPeriodToTicks(pwm, period);
    pwm->period = period;
    pwm->htim->period_ticks = ticks;
    DL_Timer_setLoadValue(pwm->htim->Instance, ticks);
}

void PWMSetDutyRatio(PWMInstance *pwm, float dutyratio)
{
    if ((pwm == NULL) || (pwm->htim == NULL) || (pwm->htim->Instance == NULL)) {
        return;
    }

    if (dutyratio < 0.0f) {
        dutyratio = 0.0f;
    } else if (dutyratio > 1.0f) {
        dutyratio = 1.0f;
    }

    uint32_t period_ticks = pwm->htim->period_ticks;
    if (period_ticks == 0U) {
        period_ticks = DL_Timer_getLoadValue(pwm->htim->Instance);
    }

    uint32_t compare = (uint32_t) ((float) period_ticks * dutyratio);
    DL_Timer_setCaptureCompareValue(
        pwm->htim->Instance, compare, pwm->channel);
    pwm->dutyratio = dutyratio;
}

void PWMStartDMA(PWMInstance *pwm, uint32_t *pData, uint32_t Size)
{
    (void) pData;
    (void) Size;

    PWMStart(pwm);
    if ((pwm != NULL) && (pwm->callback != NULL)) {
        pwm->callback(pwm);
    }
}

static uint32_t PWMSelectTclk(TIM_HandleTypeDef *htim)
{
    if ((htim == NULL) || (htim->tclk_hz == 0U)) {
        return CPUCLK_FREQ;
    }

    return htim->tclk_hz;
}

static uint32_t PWMPeriodToTicks(PWMInstance *pwm, float period)
{
    if ((period <= 0.0f) || (pwm == NULL) || (pwm->tclk == 0U)) {
        return 1U;
    }

    float ticks_f = period * (float) pwm->tclk;
    if (ticks_f < 1.0f) {
        return 1U;
    }

    if (ticks_f > 4294967295.0f) {
        return 0xFFFFFFFFU;
    }

    return (uint32_t) ticks_f;
}
