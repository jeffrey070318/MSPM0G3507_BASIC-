#include "drv8701e_driver.h"

#include <stddef.h>

static void DRV8701ESetOutput(Motor_Driver_t *driver, float output);
static void DRV8701EStop(Motor_Driver_t *driver);

static const Motor_Driver_Ops_t drv8701e_ops = {
    .set_output = DRV8701ESetOutput,
    .stop = DRV8701EStop,
};

static float ClampMagnitude(float output)
{
    float magnitude = (output < 0.0f) ? -output : output;
    return (magnitude > 1.0f) ? 1.0f : magnitude;
}

bool DRV8701EDriver_Init(Motor_Driver_t *driver,
    const DRV8701E_Driver_Init_Config_t *config)
{
    if ((driver == NULL) || (config == NULL) ||
        (config->pwm_handle == NULL) ||
        (config->pwm_handle->Instance == NULL) ||
        !(config->pwm_period > 0.0f) || (config->phase_port == NULL) ||
        (config->phase_pin == 0U)) {
        return false;
    }

    GPIO_Init_Config_s phase_config = {
        .GPIOx = config->phase_port,
        .GPIO_Pin = config->phase_pin,
        .pin_state = GPIO_PIN_RESET,
        .exti_mode = GPIO_EXTI_MODE_NONE,
        .gpio_model_callback = NULL,
        .id = NULL,
    };
    DRV8701E_Driver_t *context = &driver->context.drv8701e;
    context->phase = GPIORegister(&phase_config);
    if (context->phase == NULL) {
        return false;
    }

    PWM_Init_Config_s pwm_config = {
        .htim = config->pwm_handle,
        .channel = config->pwm_channel,
        .period = config->pwm_period,
        .dutyratio = 0.0f,
        .callback = NULL,
        .id = NULL,
    };
    context->enable_pwm = PWMRegister(&pwm_config);
    if (context->enable_pwm == NULL) {
        return false;
    }

    context->reverse = config->reverse;
    context->direction_initialized = false;
    driver->ops = &drv8701e_ops;
    DRV8701EStop(driver);
    return true;
}

static void DRV8701ESetOutput(Motor_Driver_t *driver, float output)
{
    DRV8701E_Driver_t *context = &driver->context.drv8701e;
    if (!(output > 0.0f) && !(output < 0.0f)) {
        DRV8701EStop(driver);
        return;
    }

    bool forward = (output > 0.0f) != context->reverse;
    if (!context->direction_initialized || (forward != context->forward)) {
        PWMSetDutyRatio(context->enable_pwm, 0.0f);
        if (forward) {
            GPIOSet(context->phase);
        } else {
            GPIOReset(context->phase);
        }
        context->forward = forward;
        context->direction_initialized = true;
    }

    PWMStart(context->enable_pwm);
    PWMSetDutyRatio(context->enable_pwm, ClampMagnitude(output));
}

static void DRV8701EStop(Motor_Driver_t *driver)
{
    DRV8701E_Driver_t *context = &driver->context.drv8701e;
    PWMStart(context->enable_pwm);
    PWMSetDutyRatio(context->enable_pwm, 0.0f);
}
