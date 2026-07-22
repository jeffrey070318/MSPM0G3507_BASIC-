#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_PWM

#include "bsp_pwm.h"

volatile uint32_t hardware_test_pwm_update_count;
volatile uint8_t hardware_test_pwm_duty_percent;

static PWMInstance *hardware_test_pwm;
static uint8_t hardware_test_pwm_duty_index;

Device_Status_e HardwareTestInit(void)
{
    PWM_Init_Config_s pwm_config = {
        .htim = &htim3,
        .channel = TIM_CHANNEL_2,
        .period = 0.001f,
        .dutyratio = 0.1f,
        .callback = NULL,
        .id = NULL,
    };

    hardware_test_pwm = PWMRegister(&pwm_config);
    return (hardware_test_pwm != NULL) ? DEVICE_OK : DEVICE_ERROR;
}

void HardwareTestRun(void)
{
    static const float duty_cycle[] = {0.1f, 0.5f, 0.9f};
    static const uint8_t duty_percent[] = {10U, 50U, 90U};

    PWMSetDutyRatio(
        hardware_test_pwm, duty_cycle[hardware_test_pwm_duty_index]);
    hardware_test_pwm_duty_percent =
        duty_percent[hardware_test_pwm_duty_index];
    hardware_test_pwm_update_count++;
    hardware_test_pwm_duty_index =
        (uint8_t) ((hardware_test_pwm_duty_index + 1U) % 3U);
}

#endif
