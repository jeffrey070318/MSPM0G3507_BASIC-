#include "motor.h"

#include <stddef.h>
#include <string.h>

static float MotorClampOutput(float output)
{
    if (!(output > 0.0f) && !(output < 0.0f)) {
        return 0.0f;
    }
    if (output > 1.0f) {
        return 1.0f;
    }
    if (output < -1.0f) {
        return -1.0f;
    }
    return output;
}

bool Motor_Init(
    Motor_Device_t *motor, const Motor_Init_Config_t *config)
{
    if ((motor == NULL) || (config == NULL) || (config->encoder == NULL) ||
        !(config->speed_pid.output_limit > 0.0f) ||
        (config->speed_pid.output_limit > 1.0f)) {
        return false;
    }

    memset(motor, 0, sizeof(*motor));
    if (!PID_ControllerInit(&motor->speed_pid, &config->speed_pid)) {
        return false;
    }
    if (!MotorDriver_Init(&motor->driver, &config->driver)) {
        memset(motor, 0, sizeof(*motor));
        return false;
    }

    motor->encoder = config->encoder;
    Encoder_SetReverse(motor->encoder, config->encoder_reverse);
    Encoder_Start(motor->encoder);
    motor->control_mode = MOTOR_CONTROL_SPEED;
    motor->enabled = true;
    motor->initialized = true;
    MotorDriver_Stop(&motor->driver);
    return true;
}

void Motor_SetOpenLoop(Motor_Device_t *motor, float output)
{
    if ((motor == NULL) || !motor->initialized) {
        return;
    }

    motor->control_mode = MOTOR_CONTROL_OPEN_LOOP;
    motor->control_output = MotorClampOutput(output);
    if (motor->enabled) {
        MotorDriver_SetOutput(&motor->driver, motor->control_output);
    }
}

void Motor_SetTargetSpeed(Motor_Device_t *motor, float counts_per_second)
{
    if ((motor == NULL) || !motor->initialized) {
        return;
    }

    motor->target_speed = counts_per_second;
    motor->control_mode = MOTOR_CONTROL_SPEED;
}

bool Motor_Update(Motor_Device_t *motor, float dt_seconds)
{
    if ((motor == NULL) || !motor->initialized || (motor->encoder == NULL)) {
        return false;
    }

    Encoder_Update(motor->encoder);
    if (!motor->enabled) {
        MotorDriver_Stop(&motor->driver);
        return false;
    }
    if (!(dt_seconds > 0.0f)) {
        motor->control_output = 0.0f;
        PID_ControllerReset(&motor->speed_pid);
        MotorDriver_Stop(&motor->driver);
        return false;
    }

    motor->measured_speed =
        (float) Encoder_Get_Speed(motor->encoder) / dt_seconds;
    if (motor->control_mode == MOTOR_CONTROL_SPEED) {
        motor->control_output = PID_ControllerUpdate(&motor->speed_pid,
            motor->target_speed, motor->measured_speed, dt_seconds);
        MotorDriver_SetOutput(&motor->driver, motor->control_output);
    }

    return true;
}

void Motor_Enable(Motor_Device_t *motor)
{
    if ((motor == NULL) || !motor->initialized) {
        return;
    }

    motor->target_speed = 0.0f;
    motor->measured_speed = 0.0f;
    motor->control_output = 0.0f;
    motor->control_mode = MOTOR_CONTROL_SPEED;
    motor->enabled = true;
    PID_ControllerReset(&motor->speed_pid);
    Encoder_Update(motor->encoder);
    MotorDriver_Stop(&motor->driver);
}

void Motor_Stop(Motor_Device_t *motor)
{
    if ((motor == NULL) || !motor->initialized) {
        return;
    }

    motor->target_speed = 0.0f;
    motor->control_output = 0.0f;
    motor->enabled = false;
    PID_ControllerReset(&motor->speed_pid);
    MotorDriver_Stop(&motor->driver);
}
