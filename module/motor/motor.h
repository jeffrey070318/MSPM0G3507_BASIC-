#include "smartcar_def.h"
#ifndef MOTOR_H
#define MOTOR_H

#include "stdint.h"
#include <stdbool.h>
#include "bsp_pwm.h"
#include "bsp_gpio.h"
/* 1. 物理电机对象结构体：高度解�?*/
typedef struct
{
    PWM_Device_t *pwm_pin;  //  PWM 引脚
    GPIO_Device_t *dir_in1; // 逻辑引脚 1
    GPIO_Device_t *dir_in2; // 逻辑引脚 2
    bool reverse;           // 是否反转方向
} Motor_Device_t;

void Motor_Init(Motor_Device_t *motor);

// 设置电机速度�?正数前进，负数后退�?停止)
void Motor_SetSpeed(Motor_Device_t *motor, int32_t speed);

#endif