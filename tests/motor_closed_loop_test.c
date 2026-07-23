#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "motor.h"

static bool fail_pwm_register;

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static void AssertNear(float actual, float expected)
{
    assert(AbsFloat(actual - expected) < 0.0001f);
}

GPIOInstance *GPIORegister(GPIO_Init_Config_s *config)
{
    if (config == NULL) {
        return NULL;
    }
    GPIOInstance *instance = calloc(1U, sizeof(*instance));
    assert(instance != NULL);
    instance->GPIOx = config->GPIOx;
    instance->GPIO_Pin = config->GPIO_Pin;
    instance->pin_state = config->pin_state;
    return instance;
}

void GPIOSet(GPIOInstance *instance)
{
    instance->pin_state = GPIO_PIN_SET;
}

void GPIOReset(GPIOInstance *instance)
{
    instance->pin_state = GPIO_PIN_RESET;
}

PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    if (fail_pwm_register || (config == NULL)) {
        return NULL;
    }
    PWMInstance *instance = calloc(1U, sizeof(*instance));
    assert(instance != NULL);
    instance->htim = config->htim;
    instance->channel = config->channel;
    instance->period = config->period;
    instance->dutyratio = config->dutyratio;
    instance->running = true;
    return instance;
}

void PWMStart(PWMInstance *instance)
{
    instance->running = true;
}

void PWMStop(PWMInstance *instance)
{
    instance->running = false;
}

void PWMSetDutyRatio(PWMInstance *instance, float dutyratio)
{
    instance->dutyratio = dutyratio;
}

void Encoder_Start(Encoder_Device_t *encoder)
{
    encoder->started = true;
    encoder->start_count++;
    encoder->update_count = 0U;
}

void Encoder_Update(Encoder_Device_t *encoder)
{
    encoder->update_count++;
}

void Encoder_SetReverse(Encoder_Device_t *encoder, bool reverse)
{
    encoder->reverse = reverse;
}

int16_t Encoder_Get_Speed(const Encoder_Device_t *encoder)
{
    return encoder->reverse ? (int16_t) -encoder->speed : encoder->speed;
}

static Motor_Init_Config_t MotorConfig(TIM_HandleTypeDef *timer,
    GPIO_TypeDef *gpio, Encoder_Device_t *encoder)
{
    Motor_Init_Config_t config = {
        .driver = {
            .type = MOTOR_DRIVER_DRV8701E,
            .config.drv8701e = {
                .pwm_handle = timer,
                .pwm_channel = timer->Channel,
                .pwm_period = 0.00005f,
                .phase_port = gpio,
                .phase_pin = 1U,
            },
        },
        .encoder = encoder,
        .encoder_reverse = false,
        .speed_pid = {
            .kp = 0.001f,
            .ki = 0.0f,
            .kd = 0.0f,
            .output_limit = 1.0f,
            .integral_limit = 0.5f,
        },
    };
    return config;
}

int main(void)
{
    GPTIMER_Regs timer_regs = {0};
    TIM_HandleTypeDef timer = {
        .Instance = &timer_regs,
        .Channel = 0U,
        .tclk_hz = 80000000U,
    };
    GPIO_TypeDef gpio = {0};
    Encoder_Device_t encoder = {.speed = 5};
    Motor_Device_t motor;
    Motor_Init_Config_t config = MotorConfig(&timer, &gpio, &encoder);

    assert(Motor_Init(&motor, &config));
    assert(motor.initialized);
    assert(motor.enabled);
    assert(motor.encoder == &encoder);
    assert(encoder.started);
    assert(encoder.start_count == 1U);

    Motor_SetTargetSpeed(&motor, 1000.0f);
    assert(Motor_Update(&motor, 0.01f));
    AssertNear(motor.measured_speed, 500.0f);
    AssertNear(motor.control_output, 0.5f);
    AssertNear(motor.driver.context.drv8701e.enable_pwm->dutyratio, 0.5f);

    Motor_SetOpenLoop(&motor, -0.25f);
    assert(motor.control_mode == MOTOR_CONTROL_OPEN_LOOP);
    AssertNear(motor.driver.context.drv8701e.enable_pwm->dutyratio, 0.25f);
    assert(Motor_Update(&motor, 0.01f));
    AssertNear(motor.control_output, -0.25f);

    Motor_Stop(&motor);
    assert(!motor.enabled);
    AssertNear(motor.driver.context.drv8701e.enable_pwm->dutyratio, 0.0f);
    Motor_Enable(&motor);
    assert(motor.enabled);
    assert(encoder.start_count == 1U);

    Motor_SetTargetSpeed(&motor, 1000.0f);
    assert(!Motor_Update(&motor, 0.0f));
    AssertNear(motor.control_output, 0.0f);
    AssertNear(motor.speed_pid.integral, 0.0f);

    fail_pwm_register = true;
    assert(!Motor_Init(&motor, &config));
    assert(!motor.initialized);

    config.encoder = NULL;
    assert(!Motor_Init(&motor, &config));
    assert(!Motor_Init(NULL, &config));

    return 0;
}
