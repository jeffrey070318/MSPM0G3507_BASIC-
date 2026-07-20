
#ifndef PID_H
#define PID_H

#include "stdint.h"

/* 定义 PID (Proportional Integral Derivative, 比例积分微分) 的两种运算模式 */
typedef enum
{
    PID_POSITION = 0, // 位置式 PID
    PID_DELTA         // 增量式 PID
} PID_Mode_e;

/* PID 核心对象结构体：彻底解耦，互不干扰 */
typedef struct
{
    PID_Mode_e mode;

    // 1. 调参核心：比例、积分、微分系数
    float Kp;
    float Ki;
    float Kd;

    // 2. 硬件保护限制（极度重要）
    float max_out;  // 最大总输出限幅 (防止输出瞬间打满烧毁电机或驱动器)
    float max_iout; // 最大积分限幅 (抗积分饱和，防止卡死堵转后松开时的突然暴走)

    // 3. 运行过程变量
    float set; // 目标设定值 (Set Point)
    float fdb; // 实际反馈值 (Feedback)

    float out;  // 最终下发给硬件的总输出值
    float Pout; // 比例项输出
    float Iout; // 积分项输出
    float Dout; // 微分项输出

    // 4. 误差追溯矩阵：记录历史偏差供微分项使用
    // error[0]: 当前误差 e(k)
    // error[1]: 上次误差 e(k-1)
    // error[2]: 上上次误差 e(k-2)
    float error[3];

} PID_Device_t;

/* --- API 极简声明 --- */

// 初始化一个 PID 对象
void PID_Init(PID_Device_t *pid, PID_Mode_e mode, float kp, float ki, float kd, float max_out, float max_iout);

// 核心控制节拍：输入目标值与实际反馈，计算并返回下发控制量
float PID_Calc(PID_Device_t *pid, float fdb, float set);

// 状态清除：用于急停或电机重新启动前，清空历史残余的积分
void PID_Clear(PID_Device_t *pid);

void PID_SetParam(PID_Device_t *pid, float kp, float ki, float kd);

#endif