#include "motor.h"

static bool MotorIsValid(const Motor_Device_t *motor)
{
    return (motor != NULL) && (motor->pwm_pin != NULL) &&
           (motor->dir_in1 != NULL) && (motor->dir_in2 != NULL);
}

static void MotorSetDirection(Motor_Device_t *motor, bool forward)
{
    bool effective_forward = (forward != motor->reverse);
    if (effective_forward) {
        GPIOSet(motor->dir_in1);
        GPIOReset(motor->dir_in2);
    } else {
        GPIOReset(motor->dir_in1);
        GPIOSet(motor->dir_in2);
    }
}

static void MotorStop(Motor_Device_t *motor)
{
    if (motor->stop_mode == MOTOR_STOP_BRAKE) {
        GPIOSet(motor->dir_in1);
        GPIOSet(motor->dir_in2);
    } else {
        GPIOReset(motor->dir_in1);
        GPIOReset(motor->dir_in2);
    }
    PWMSetDutyRatio(motor->pwm_pin, 0.0f);
}

void Motor_Init(Motor_Device_t *motor)
{
    if (!MotorIsValid(motor)) {
        return;
    }

    PWMStart(motor->pwm_pin);
    Motor_SetSpeed(motor, 0);
}

void Motor_SetSpeed(Motor_Device_t *motor, int32_t speed)
{
    if (!MotorIsValid(motor)) {
        return;
    }

    if (speed == 0) {
        MotorStop(motor);
        return;
    }

    if (speed > MOTOR_OUTPUT_MAX) {
        speed = MOTOR_OUTPUT_MAX;
    } else if (speed < -MOTOR_OUTPUT_MAX) {
        speed = -MOTOR_OUTPUT_MAX;
    }

    MotorSetDirection(motor, speed > 0);
    uint32_t magnitude =
        (speed > 0) ? (uint32_t) speed : (uint32_t) (-speed);
    PWMSetDutyRatio(
        motor->pwm_pin, (float) magnitude / (float) MOTOR_OUTPUT_MAX);
}
