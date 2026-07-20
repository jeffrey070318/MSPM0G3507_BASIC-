#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_gpio.h"
#include "bsp_pwm.h"

#define MOTOR_OUTPUT_MAX 1000

typedef enum {
    MOTOR_STOP_COAST = 0,
    MOTOR_STOP_BRAKE,
} Motor_Stop_Mode_e;

typedef struct {
    PWMInstance *pwm_pin;
    GPIOInstance *dir_in1;
    GPIOInstance *dir_in2;
    bool reverse;
    Motor_Stop_Mode_e stop_mode;
} Motor_Device_t;

void Motor_Init(Motor_Device_t *motor);
void Motor_SetSpeed(Motor_Device_t *motor, int32_t speed);

#endif
