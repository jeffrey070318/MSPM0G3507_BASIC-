#include "tb6612_driver.h"

#include <stddef.h>

static void TB6612SetOutput(Motor_Driver_t *driver, float output);
static void TB6612Stop(Motor_Driver_t *driver);

static const Motor_Driver_Ops_t tb6612_ops = {
    .set_output = TB6612SetOutput,
    .stop = TB6612Stop,
};

static float ClampMagnitude(float output)
{
    float magnitude = (output < 0.0f) ? -output : output;
    return (magnitude > 1.0f) ? 1.0f : magnitude;
}

bool TB6612Driver_Init(Motor_Driver_t *driver,
    const TB6612_Driver_Init_Config_t *config)
{
    if ((driver == NULL) || (config == NULL) ||
        (config->pwm_handle == NULL) ||
        (config->pwm_handle->Instance == NULL) ||
        !(config->pwm_period > 0.0f) ||
        (config->in1_port == NULL) || (config->in1_pin == 0U) ||
        (config->in2_port == NULL) || (config->in2_pin == 0U)) {
        return false;
    }

    TB6612_Driver_t *context = &driver->context.tb6612;
    GPIO_Init_Config_s in1_config = {
        .GPIOx = config->in1_port,
        .GPIO_Pin = config->in1_pin,
        .pin_state = GPIO_PIN_RESET,
        .exti_mode = GPIO_EXTI_MODE_NONE,
    };
    context->in1 = GPIORegister(&in1_config);
    if (context->in1 == NULL) {
        return false;
    }

    GPIO_Init_Config_s in2_config = {
        .GPIOx = config->in2_port,
        .GPIO_Pin = config->in2_pin,
        .pin_state = GPIO_PIN_RESET,
        .exti_mode = GPIO_EXTI_MODE_NONE,
    };
    context->in2 = GPIORegister(&in2_config);
    if (context->in2 == NULL) {
        return false;
    }

    PWM_Init_Config_s pwm_config = {
        .htim = config->pwm_handle,
        .channel = config->pwm_channel,
        .period = config->pwm_period,
        .dutyratio = 0.0f,
    };
    context->pwm = PWMRegister(&pwm_config);
    if (context->pwm == NULL) {
        return false;
    }

    context->reverse = config->reverse;
    context->stop_mode = config->stop_mode;
    driver->ops = &tb6612_ops;
    TB6612Stop(driver);
    return true;
}

static void TB6612SetOutput(Motor_Driver_t *driver, float output)
{
    TB6612_Driver_t *context = &driver->context.tb6612;
    if (!(output > 0.0f) && !(output < 0.0f)) {
        TB6612Stop(driver);
        return;
    }

    bool forward = (output > 0.0f) != context->reverse;
    PWMSetDutyRatio(context->pwm, 0.0f);
    if (forward) {
        GPIOSet(context->in1);
        GPIOReset(context->in2);
    } else {
        GPIOReset(context->in1);
        GPIOSet(context->in2);
    }
    PWMStart(context->pwm);
    PWMSetDutyRatio(context->pwm, ClampMagnitude(output));
}

static void TB6612Stop(Motor_Driver_t *driver)
{
    TB6612_Driver_t *context = &driver->context.tb6612;
    PWMSetDutyRatio(context->pwm, 0.0f);
    if (context->stop_mode == MOTOR_STOP_BRAKE) {
        GPIOSet(context->in1);
        GPIOSet(context->in2);
    } else {
        GPIOReset(context->in1);
        GPIOReset(context->in2);
    }
}
