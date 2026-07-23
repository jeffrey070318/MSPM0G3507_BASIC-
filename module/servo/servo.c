#include "servo.h"

#include <stddef.h>

bool SERVO_Init(SERVO_Device_t *servo, TIM_HandleTypeDef *pwm_handle,
    float min_pulse_seconds, float max_pulse_seconds)
{
    if ((servo == NULL) || (pwm_handle == NULL) ||
        (pwm_handle->Instance == NULL) ||
        !(min_pulse_seconds > 0.0f) ||
        !(max_pulse_seconds > min_pulse_seconds) ||
        !(max_pulse_seconds < SERVO_PERIOD_SECONDS)) {
        return false;
    }

    servo->pwm = NULL;
    servo->min_pulse_seconds = min_pulse_seconds;
    servo->max_pulse_seconds = max_pulse_seconds;
    servo->angle_degrees = SERVO_MIN_ANGLE_DEGREES;

    PWM_Init_Config_s config = {
        .htim = pwm_handle,
        .channel = pwm_handle->Channel,
        .period = SERVO_PERIOD_SECONDS,
        .dutyratio = 0.0f,
        .callback = NULL,
        .id = servo,
    };
    servo->pwm = PWMRegister(&config);
    if (servo->pwm == NULL) {
        return false;
    }

    PWMStop(servo->pwm);
    return true;
}

bool SERVO_SetAngle(SERVO_Device_t *servo, float angle_degrees)
{
    if ((servo == NULL) || (servo->pwm == NULL) ||
        (!(angle_degrees >= 0.0f) && !(angle_degrees < 0.0f))) {
        return false;
    }

    if (angle_degrees < SERVO_MIN_ANGLE_DEGREES) {
        angle_degrees = SERVO_MIN_ANGLE_DEGREES;
    } else if (angle_degrees > SERVO_MAX_ANGLE_DEGREES) {
        angle_degrees = SERVO_MAX_ANGLE_DEGREES;
    }

    float position = angle_degrees / SERVO_MAX_ANGLE_DEGREES;
    float pulse_seconds = servo->min_pulse_seconds +
        position * (servo->max_pulse_seconds - servo->min_pulse_seconds);
    PWMSetDutyRatio(servo->pwm, pulse_seconds / SERVO_PERIOD_SECONDS);
    PWMStart(servo->pwm);
    servo->angle_degrees = angle_degrees;
    return true;
}

void SERVO_Stop(SERVO_Device_t *servo)
{
    if ((servo != NULL) && (servo->pwm != NULL)) {
        PWMStop(servo->pwm);
    }
}
