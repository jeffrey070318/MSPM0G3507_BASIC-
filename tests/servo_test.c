#include <assert.h>
#include <stddef.h>

#include "servo.h"

static PWMInstance pwm_instance;
static bool fail_registration;
static uint32_t start_calls;
static uint32_t stop_calls;

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static void AssertNear(float actual, float expected)
{
    assert(AbsFloat(actual - expected) < 0.0001f);
}

PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    if (fail_registration || (config == NULL)) {
        return NULL;
    }
    pwm_instance.htim = config->htim;
    pwm_instance.channel = config->channel;
    pwm_instance.period = config->period;
    pwm_instance.dutyratio = config->dutyratio;
    pwm_instance.running = true;
    return &pwm_instance;
}

void PWMStart(PWMInstance *instance)
{
    instance->running = true;
    start_calls++;
}

void PWMStop(PWMInstance *instance)
{
    instance->running = false;
    stop_calls++;
}

void PWMSetDutyRatio(PWMInstance *instance, float duty_ratio)
{
    instance->dutyratio = duty_ratio;
}

int main(void)
{
    GPTIMER_Regs timer_regs = {0};
    TIM_HandleTypeDef timer = {
        .Instance = &timer_regs,
        .Channel = 0U,
        .tclk_hz = 1000000U,
        .period_ticks = 20000U,
        .count_up = true,
    };
    SERVO_Device_t servo = {0};
    SERVO_Device_t unused_servo = {0};

    assert(SERVO_Init(&servo, &timer, 0.0005f, 0.0025f));
    AssertNear(pwm_instance.period, 0.020f);
    assert(!pwm_instance.running);
    assert(stop_calls == 1U);

    assert(SERVO_SetAngle(&servo, 0.0f));
    AssertNear(pwm_instance.dutyratio, 0.025f);
    AssertNear(servo.angle_degrees, 0.0f);
    assert(pwm_instance.running);

    assert(SERVO_SetAngle(&servo, 90.0f));
    AssertNear(pwm_instance.dutyratio, 0.075f);
    AssertNear(servo.angle_degrees, 90.0f);

    assert(SERVO_SetAngle(&servo, 200.0f));
    AssertNear(pwm_instance.dutyratio, 0.125f);
    AssertNear(servo.angle_degrees, 180.0f);

    assert(SERVO_SetAngle(&servo, -20.0f));
    AssertNear(pwm_instance.dutyratio, 0.025f);
    AssertNear(servo.angle_degrees, 0.0f);
    assert(start_calls == 4U);

    SERVO_Stop(&servo);
    assert(!pwm_instance.running);
    assert(stop_calls == 2U);

    assert(!SERVO_SetAngle(NULL, 90.0f));
    assert(!SERVO_SetAngle(&unused_servo, 90.0f));
    SERVO_Stop(NULL);
    SERVO_Stop(&unused_servo);

    assert(!SERVO_Init(NULL, &timer, 0.0005f, 0.0025f));
    assert(!SERVO_Init(&unused_servo, NULL, 0.0005f, 0.0025f));
    assert(!SERVO_Init(&unused_servo, &timer, 0.0f, 0.0025f));
    assert(!SERVO_Init(&unused_servo, &timer, 0.0025f, 0.0005f));
    assert(!SERVO_Init(&unused_servo, &timer, 0.0005f, 0.020f));

    fail_registration = true;
    assert(!SERVO_Init(&unused_servo, &timer, 0.0005f, 0.0025f));
    return 0;
}
