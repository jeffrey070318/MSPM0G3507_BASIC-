#include "bsp_pwm.h"
#include "bsp_memory.h"

#include <string.h>

static uint8_t idx;
static PWMInstance *pwm_instance[PWM_DEVICE_CNT] = {NULL};

static uint32_t PWMEnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void PWMExitCritical(uint32_t primask)
{
    if (primask == 0U) {
        __enable_irq();
    }
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

static uint32_t PWMDutyToCompare(
    const TIM_HandleTypeDef *htim, float dutyratio)
{
    float active_ticks = ((float) htim->period_ticks + 1.0f) *
                         (htim->count_up ? dutyratio : (1.0f - dutyratio));
    if (active_ticks <= 1.0f) {
        return 0U;
    }
    if (active_ticks >= ((float) htim->period_ticks + 1.0f)) {
        return htim->period_ticks;
    }
    return (uint32_t) (active_ticks + 0.5f) - 1U;
}

static void PWMApplyOutput(PWMInstance *pwm)
{
    float output_duty = pwm->running ? pwm->dutyratio : 0.0f;
    uint32_t compare = PWMDutyToCompare(pwm->htim, output_duty);
    DL_Timer_setCaptureCompareValue(
        pwm->htim->Instance, compare, (DL_TIMER_CC_INDEX) pwm->channel);
}

static bool PWMAnyChannelRunning(
    GPTIMER_Regs *timer, const PWMInstance *except)
{
    for (uint8_t i = 0U; i < idx; ++i) {
        PWMInstance *candidate = pwm_instance[i];
        if ((candidate != NULL) && (candidate != except) &&
            (candidate->htim != NULL) &&
            (candidate->htim->Instance == timer) && candidate->running) {
            return true;
        }
    }
    return false;
}

PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    if ((config == NULL) || (config->htim == NULL) ||
        (config->htim->Instance == NULL) ||
        !(config->period > 0.0f) ||
        (idx >= PWM_DEVICE_CNT)) {
        return NULL;
    }
    for (uint8_t i = 0U; i < idx; ++i) {
        if ((pwm_instance[i] != NULL) &&
            (pwm_instance[i]->htim != NULL) &&
            (pwm_instance[i]->htim->Instance == config->htim->Instance) &&
            (pwm_instance[i]->channel == config->channel)) {
            return NULL;
        }
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
    pwm->running = false;
    pwm->callback = config->callback;
    pwm->id = config->id;

    if (pwm->tclk == 0U) {
        BSPFree(pwm);
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
        uint32_t primask = PWMEnterCritical();
        pwm->running = true;
        PWMApplyOutput(pwm);
        DL_Timer_startCounter(pwm->htim->Instance);
        PWMExitCritical(primask);
    }
}

void PWMStop(PWMInstance *pwm)
{
    if ((pwm != NULL) && (pwm->htim != NULL) &&
        (pwm->htim->Instance != NULL)) {
        uint32_t primask = PWMEnterCritical();
        pwm->running = false;
        PWMApplyOutput(pwm);
        if (!PWMAnyChannelRunning(pwm->htim->Instance, pwm)) {
            DL_Timer_stopCounter(pwm->htim->Instance);
        }
        PWMExitCritical(primask);
    }
}

void PWMSetPeriod(PWMInstance *pwm, float period)
{
    if ((pwm == NULL) || (pwm->htim == NULL) ||
        (pwm->htim->Instance == NULL) || !(period > 0.0f)) {
        return;
    }

    uint32_t period_ticks = PWMPeriodToTicks(pwm, period);
    uint32_t primask = PWMEnterCritical();

    pwm->htim->period_ticks = period_ticks;
    pwm->period = period;
    DL_Timer_setLoadValue(pwm->htim->Instance, period_ticks);
    PWMApplyOutput(pwm);

    for (uint8_t i = 0U; i < idx; ++i) {
        PWMInstance *peer = pwm_instance[i];
        if ((peer == NULL) || (peer == pwm) || (peer->htim == NULL) ||
            (peer->htim->Instance != pwm->htim->Instance)) {
            continue;
        }
        peer->htim->period_ticks = period_ticks;
        peer->period = period;
        PWMApplyOutput(peer);
    }

    PWMExitCritical(primask);
}

void PWMSetDutyRatio(PWMInstance *pwm, float dutyratio)
{
    if ((pwm == NULL) || (pwm->htim == NULL) ||
        (pwm->htim->Instance == NULL)) {
        return;
    }

    if (!(dutyratio >= 0.0f)) {
        dutyratio = 0.0f;
    } else if (dutyratio > 1.0f) {
        dutyratio = 1.0f;
    }

    uint32_t primask = PWMEnterCritical();
    pwm->dutyratio = dutyratio;
    PWMApplyOutput(pwm);
    PWMExitCritical(primask);
}

void PWMStartDMA(PWMInstance *pwm, uint32_t *pData, uint32_t Size)
{
    (void) pData;
    (void) Size;

    /* No PWM DMA channel is currently owned by SysConfig. */
    PWMStart(pwm);
}
