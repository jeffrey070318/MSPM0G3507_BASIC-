/**
 * @file pid.c
 * @cite 玺佬
 * @date 2026-3-8
 *
 */
#include "pid.h"
#include <stddef.h> // 提供 NULL 宏的定义

/**
 * @brief 限幅保护辅助函数 (静态私有函数)
 * @note  底层原理：当传入的运算结果超出安全阈值时，强制将其“削平”截断。
 */
static float PID_Limit(float val, float max)
{
    if (val > max)
        return max;
    if (val < -max)
        return -max;
    return val;
}

/**
 * @brief 初始化 PID 对象的内部参数
 */
void PID_Init(PID_Device_t *pid, PID_Mode_e mode, float kp, float ki, float kd, float max_out, float max_iout)
{
    if (pid == NULL)
        return;

    pid->mode = mode;
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;

    pid->max_out = max_out;
    pid->max_iout = max_iout;

    PID_Clear(pid);
}

/**
 * @brief 清除历史状态内存
 * @note  底层原理：每次电机停转后再启动时，如果没有清零，之前的误差积分（Iout）会导致车辆瞬间失控。
 */
void PID_Clear(PID_Device_t *pid)
{
    if (pid == NULL)
        return;

    pid->set = 0.0f;
    pid->fdb = 0.0f;
    pid->out = 0.0f;
    pid->Pout = 0.0f;
    pid->Iout = 0.0f;
    pid->Dout = 0.0f;

    pid->error[0] = 0.0f;
    pid->error[1] = 0.0f;
    pid->error[2] = 0.0f;
}

/**
 * @brief 核心控制算法：计算下一拍的控制量
 * @param fdb 传感器的实际反馈值 (例如编码器读到的真实速度)
 * @param set 你的设定目标值 (例如期望跑出的目标速度)
 */
float PID_Calc(PID_Device_t *pid, float fdb, float set)
{
    if (pid == NULL)
        return 0.0f;

    // 1. 刷新设定与反馈
    pid->set = set;
    pid->fdb = fdb;

    // 2. 误差历史矩阵平移更新
    pid->error[2] = pid->error[1]; // 记录 e(k-2)
    pid->error[1] = pid->error[0]; // 记录 e(k-1)
    pid->error[0] = set - fdb;     // 计算当前最新误差 e(k)

    // 3. 根据所选模式，执行底层数学解算
    if (pid->mode == PID_POSITION)
    {
        /* * 位置式 PID：最常用的算法。直接算出当前应该给底层的绝对控制量。
         * 公式：out = Kp * e(k) + Ki * Σe(n) + Kd * [e(k) - e(k-1)]
         */
        pid->Pout = pid->Kp * pid->error[0];
        pid->Iout += pid->Ki * pid->error[0]; // 积分项不断累加误差
        pid->Dout = pid->Kd * (pid->error[0] - pid->error[1]);

        // 【安全核心】抗积分饱和：限制积分项的无限膨胀
        pid->Iout = PID_Limit(pid->Iout, pid->max_iout);

        pid->out = pid->Pout + pid->Iout + pid->Dout;
    }
    else if (pid->mode == PID_DELTA)
    {
        /* * 增量式 PID：算出的是控制量的“变化值”。适用于不允许控制量突变的场景。
         * 公式：Δout = Kp * [e(k) - e(k-1)] + Ki * e(k) + Kd * [e(k) - 2e(k-1) + e(k-2)]
         */
        pid->Pout = pid->Kp * (pid->error[0] - pid->error[1]);
        pid->Iout = pid->Ki * pid->error[0];
        pid->Dout = pid->Kd * (pid->error[0] - 2.0f * pid->error[1] + pid->error[2]);

        // 结果叠加：当前输出 = 上次输出 + 本次变化量
        pid->out += pid->Pout + pid->Iout + pid->Dout;
    }

    // 4. 总输出限幅：保护电机硬件
    pid->out = PID_Limit(pid->out, pid->max_out);

    return pid->out;
}

/**
 * @brief 动态重载 PID 参数 (专为在线调参设计)
 * @param kp 比例系数
 * @param ki 积分系数
 * @param kd 微分系数
 */
void PID_SetParam(PID_Device_t *pid, float kp, float ki, float kd)
{
    if (pid == NULL)
        return;

    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
}