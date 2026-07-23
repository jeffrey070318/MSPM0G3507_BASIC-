#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "motor_driver.h"

static bool fail_pwm_register;
static uint32_t gpio_register_calls;
static uint32_t fail_gpio_register_call;
static float duty_history[32];
static uint32_t duty_history_count;

static void ResetFakes(void)
{
    fail_pwm_register = false;
    gpio_register_calls = 0U;
    fail_gpio_register_call = 0U;
    duty_history_count = 0U;
}

GPIOInstance *GPIORegister(GPIO_Init_Config_s *config)
{
    gpio_register_calls++;
    if ((config == NULL) ||
        (gpio_register_calls == fail_gpio_register_call)) {
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
    assert(duty_history_count < 32U);
    duty_history[duty_history_count++] = dutyratio;
}

static Motor_Driver_Init_Config_t DRVConfig(
    TIM_HandleTypeDef *timer, GPIO_TypeDef *port)
{
    Motor_Driver_Init_Config_t config = {
        .type = MOTOR_DRIVER_DRV8701E,
        .config.drv8701e = {
            .pwm_handle = timer,
            .pwm_channel = timer->Channel,
            .pwm_period = 0.00005f,
            .phase_port = port,
            .phase_pin = 1U,
            .reverse = false,
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
    Motor_Driver_t driver;

    ResetFakes();
    Motor_Driver_Init_Config_t drv_config = DRVConfig(&timer, &gpio);
    assert(MotorDriver_Init(&driver, &drv_config));
    MotorDriver_SetOutput(&driver, 0.6f);
    assert(driver.context.drv8701e.phase->pin_state == GPIO_PIN_SET);
    assert(driver.context.drv8701e.enable_pwm->dutyratio == 0.6f);

    uint32_t history_before_reverse = duty_history_count;
    MotorDriver_SetOutput(&driver, -0.4f);
    assert(duty_history[history_before_reverse] == 0.0f);
    assert(driver.context.drv8701e.phase->pin_state == GPIO_PIN_RESET);
    assert(driver.context.drv8701e.enable_pwm->dutyratio == 0.4f);
    MotorDriver_Stop(&driver);
    assert(driver.context.drv8701e.enable_pwm->dutyratio == 0.0f);
    ResetFakes();
    Motor_Driver_Init_Config_t tb_config = {
        .type = MOTOR_DRIVER_TB6612,
        .config.tb6612 = {
            .pwm_handle = &timer,
            .pwm_channel = timer.Channel,
            .pwm_period = 0.00005f,
            .in1_port = &gpio,
            .in1_pin = 1U,
            .in2_port = &gpio,
            .in2_pin = 2U,
            .reverse = false,
            .stop_mode = MOTOR_STOP_COAST,
        },
    };
    assert(MotorDriver_Init(&driver, &tb_config));
    MotorDriver_SetOutput(&driver, 0.5f);
    assert(driver.context.tb6612.in1->pin_state == GPIO_PIN_SET);
    assert(driver.context.tb6612.in2->pin_state == GPIO_PIN_RESET);
    MotorDriver_SetOutput(&driver, -0.25f);
    assert(driver.context.tb6612.in1->pin_state == GPIO_PIN_RESET);
    assert(driver.context.tb6612.in2->pin_state == GPIO_PIN_SET);
    MotorDriver_Stop(&driver);
    assert(driver.context.tb6612.in1->pin_state == GPIO_PIN_RESET);
    assert(driver.context.tb6612.in2->pin_state == GPIO_PIN_RESET);
    ResetFakes();
    tb_config.config.tb6612.stop_mode = MOTOR_STOP_BRAKE;
    assert(MotorDriver_Init(&driver, &tb_config));
    MotorDriver_Stop(&driver);
    assert(driver.context.tb6612.in1->pin_state == GPIO_PIN_SET);
    assert(driver.context.tb6612.in2->pin_state == GPIO_PIN_SET);
    ResetFakes();
    fail_pwm_register = true;
    assert(!MotorDriver_Init(&driver, &drv_config));

    ResetFakes();
    fail_gpio_register_call = 2U;
    assert(!MotorDriver_Init(&driver, &tb_config));

    assert(!MotorDriver_Init(NULL, &drv_config));
    assert(!MotorDriver_Init(&driver, NULL));

    return 0;
}
