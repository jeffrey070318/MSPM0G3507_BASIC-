// User/module/motor/motor.c
#include "motor.h"
#include "smartcar_def.h" // 引入全局物理极限宏定义

/**
 * @brief 初始化电机，开启硬件通道
 */
void Motor_Init(Motor_Device_t *motor)
{
    if (motor == NULL)
        return;

    // 启动底层的定时器 PWM 通道
    PWM_Start(motor->pwm_pin);

    // 默认让电机处于停止状态
    Motor_SetSpeed(motor, 0);
}

/**
 * @brief 控制电机转动
 * @param speed 取值范围：-MAX_MOTOR_PWM 到 +MAX_MOTOR_PWM
 */
void Motor_SetSpeed(Motor_Device_t *motor, int32_t speed)
{
    if (motor == NULL)
        return;

    // 1. 安全限幅：绝不允许传入的数值超过定时器 ARR 的最大值
    if (speed > MAX_MOTOR_PWM)
        speed = MAX_MOTOR_PWM;
    if (speed < -MAX_MOTOR_PWM)
        speed = -MAX_MOTOR_PWM;

    // 2. 方向解算与底层驱动下发
    if (speed > 0)
    {
        if (motor->reverse)
        {
            // 反转电机：正转逻辑变为反转的 GPIO
            GPIO_Low(motor->dir_in1);
            GPIO_High(motor->dir_in2);
        }
        else
        {
            // 正转逻辑：IN1高，IN2低
            GPIO_High(motor->dir_in1);
            GPIO_Low(motor->dir_in2);
        }
        // 下发绝对值作为占空比
        PWM_SetCompare(motor->pwm_pin, (uint32_t)speed);
    }
    else if (speed < 0)
    {
        if (motor->reverse)
        {
            // 反转电机：反转逻辑变为正转的 GPIO
            GPIO_High(motor->dir_in1);
            GPIO_Low(motor->dir_in2);
        }
        else
        {
            // 反转逻辑：IN1低，IN2高
            GPIO_Low(motor->dir_in1);
            GPIO_High(motor->dir_in2);
        }
        // 负数转正数后下发
        PWM_SetCompare(motor->pwm_pin, (uint32_t)(-speed));
    }
    else
    {
        // 刹车停止逻辑：双高
        GPIO_High(motor->dir_in1);
        GPIO_High(motor->dir_in2);
        PWM_SetCompare(motor->pwm_pin, 0);
    }
}