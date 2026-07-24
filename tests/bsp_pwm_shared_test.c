#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "bsp_pwm.h"

void DL_Timer_startCounter(GPTIMER_Regs *timer)
{
    timer->running = true;
    timer->start_count++;
}

void DL_Timer_stopCounter(GPTIMER_Regs *timer)
{
    timer->running = false;
    timer->stop_count++;
}

void DL_Timer_setLoadValue(GPTIMER_Regs *timer, uint32_t load)
{
    timer->load = load;
}

void DL_Timer_setCaptureCompareValue(
    GPTIMER_Regs *timer, uint32_t compare, DL_TIMER_CC_INDEX channel)
{
    assert(channel < 4U);
    timer->compare[channel] = compare;
}

int main(void)
{
    GPTIMER_Regs timer = {0};
    TIM_HandleTypeDef left_handle = {
        .Instance = &timer,
        .Channel = 0U,
        .tclk_hz = 1000U,
        .period_ticks = 10U,
        .count_up = false,
    };
    TIM_HandleTypeDef right_handle = {
        .Instance = &timer,
        .Channel = 1U,
        .tclk_hz = 1000U,
        .period_ticks = 10U,
        .count_up = false,
    };
    PWM_Init_Config_s left_config = {
        .htim = &left_handle,
        .channel = left_handle.Channel,
        .period = 0.010f,
        .dutyratio = 0.25f,
    };
    PWM_Init_Config_s right_config = {
        .htim = &right_handle,
        .channel = right_handle.Channel,
        .period = 0.010f,
        .dutyratio = 0.75f,
    };

    PWMInstance *left = PWMRegister(&left_config);
    PWMInstance *right = PWMRegister(&right_config);
    assert(left != NULL);
    assert(right != NULL);
    assert(timer.running);

    PWMSetPeriod(left, 0.020f);
    assert(timer.load == 20U);
    assert(left_handle.period_ticks == 20U);
    assert(right_handle.period_ticks == 20U);
    assert(timer.compare[1] == 4U);

    PWMStop(left);
    assert(timer.running);
    assert(timer.compare[0] == 20U);

    PWMStart(left);
    assert(timer.running);
    assert(timer.compare[0] == 15U);

    PWMStop(left);
    PWMStop(right);
    assert(!timer.running);
    assert(timer.stop_count == 1U);

    assert(PWMRegister(&left_config) == NULL);
    PWM_Init_Config_s invalid_period = left_config;
    invalid_period.period = 0.0f;
    assert(PWMRegister(&invalid_period) == NULL);

    return 0;
}
