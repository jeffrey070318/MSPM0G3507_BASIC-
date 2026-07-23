#ifndef MODULE_MOTOR_H
#define MODULE_MOTOR_H

#include <stdbool.h>

#include "algorithm/pid.h"
#include "bsp_encoder.h"
#include "motor_driver.h"

typedef enum {
    MOTOR_CONTROL_SPEED = 0,
    MOTOR_CONTROL_OPEN_LOOP,
} Motor_Control_Mode_e;

typedef struct {
    Motor_Driver_Init_Config_t driver;
    Encoder_Device_t *encoder;
    PID_Config_t speed_pid;
    bool encoder_reverse;
} Motor_Init_Config_t;

typedef struct {
    Motor_Driver_t driver;
    Encoder_Device_t *encoder;
    PID_Controller_t speed_pid;
    float target_speed;
    float measured_speed;
    float control_output;
    Motor_Control_Mode_e control_mode;
    bool enabled;
    bool initialized;
} Motor_Device_t;

bool Motor_Init(
    Motor_Device_t *motor, const Motor_Init_Config_t *config);
void Motor_SetOpenLoop(Motor_Device_t *motor, float output);
void Motor_SetTargetSpeed(Motor_Device_t *motor, float counts_per_second);
bool Motor_Update(Motor_Device_t *motor, float dt_seconds);
void Motor_Enable(Motor_Device_t *motor);
void Motor_Stop(Motor_Device_t *motor);

#endif
