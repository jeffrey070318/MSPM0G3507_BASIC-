#ifndef _INS_H_
#define _INS_H_

#include <stdint.h>
#include <stdbool.h>
#include "imu.h"
#include "pid.h"

/* ================================================================
 *                    数学常量
 * ================================================================ */
#define MY_PI         3.14159265f        /* 圆周率 π */
#define DEG_TO_RAD    0.01745329252f     /* 角度转弧度乘数: π/180 */

/* ================================================================
 *                    阈值
 * ================================================================ */
#define TARGET_DISTANCE   0.05f          /* 到达原点阈值: 5 cm */

/* ================================================================
 *                    消息中心话题名
 * ================================================================ */
#define INS_ENCODER_TOPIC  "encoder_data"  /* 订阅: 底盘发布的编码器数据 */
#define INS_CMD_TOPIC      "chassis_cmd"   /* 发布: 发给底盘的速度指令 */

/* ================================================================
 *                    小车物理参数（按实际修改）
 * ================================================================ */
#define INS_WHEEL_RADIUS_M   0.076f        /* 轮子半径 (m) */
#define INS_TRACK_WIDTH_M    0.40f         /* 左右轮轴距 (m) */
#define INS_ENCODER_PPR      11.0f         /* 编码器线数 (每圈脉冲数) */
#define INS_GEAR_RATIO       19.0f         /* 电机减速比 */

/* 每个编码器脉冲对应的小车位移 (m) */
#define INS_METERS_PER_COUNT \
    (6.283185307f * INS_WHEEL_RADIUS_M / (INS_ENCODER_PPR * 4.0f * INS_GEAR_RATIO))

/* ================================================================
 *                    PID 参数（返航用，需实测调参）
 * ================================================================ */

/* 距离环: dist → vx (线速度 m/s) */
#define INS_DIST_KP        0.3f            /* 比例: 越远越快 */
#define INS_DIST_KI        0.015f          /* 积分: 消除静差 */
#define INS_DIST_KD        0.0f            /* 微分: 一般不用 */
#define INS_DIST_MAX_OUT   0.4f            /* 最大线速度 (m/s) */
#define INS_DIST_MAX_IOUT  0.3f            /* 积分限幅 */

/* 角度环: err_deg → wz (角速度 rad/s) */
#define INS_ANGLE_KP       2.5f            /* 比例: 偏差越大转越快 */
#define INS_ANGLE_KI       0.03f           /* 积分: 消除稳态偏角 */
#define INS_ANGLE_KD       0.0f            /* 微分: 一般不用 */
#define INS_ANGLE_MAX_OUT  2.0f            /* 最大角速度 (rad/s) */
#define INS_ANGLE_MAX_IOUT 1.5f            /* 积分限幅 */

/* ================================================================
 *                    类型定义
 * ================================================================ */

/* 姿态角 (robot_def.h 兼容) */
typedef struct {
    float yaw;      /* 偏航角 (deg) */
    float pitch;    /* 俯仰角 (deg) */
    float roll;     /* 横滚角 (deg) */
} attitude_t;

/* 二维位置 (m) */
typedef struct {
    float x;
    float y;
} Position_t;

/* 编码器数据 —— 底盘通过消息中心发布，INS 订阅 */
typedef struct {
    int32_t left_total;     /* 左轮累计脉冲数 */
    int32_t right_total;    /* 右轮累计脉冲数 */
} Encoder_Pub_Data_t;

/* 底盘控制指令 —— INS 通过消息中心发布，底盘订阅 */
typedef struct {
    float   vx;             /* 线速度 (m/s) */
    float   vy;             /* 横向速度, 差速底盘不用 */
    float   wz;             /* 角速度 (rad/s) */
    float   offset_angle;   /* 云台偏置, 不用 */
    uint8_t chassis_mode;   /* 0 = 卸力停止, 1 = 运动 */
    int8_t  speed_buff;
} Chassis_Cmd_Pub_t;

/* INS 内部状态机 */
typedef enum {
    INS_STATE_IDLE      = 0,  /* 空闲: 只跟踪位置, 不下发指令 */
    INS_STATE_RETURNING,      /* 返航中: PID 控车回原点 */
    INS_STATE_DONE,           /* 已到达: 停车 */
} INS_State_e;

/* 返航进度 */
typedef enum {
    NAV_TARGET_FAR         = 0,  /* 离目标还远 */
    NAV_TARGET_APPROACHING,      /* 接近中 */
    NAV_TARGET_REACHED           /* 已到达 */
} NavReachState_e;

/* ================================================================
 *                    API（共 6 个函数）
 * ================================================================ */

/* 初始化: 订阅 imu_data + encoder_data, 注册 chassis_cmd 发布者, 初始化 PID */
void INS_Init(void);

/* 主任务: 每个周期调用一次 (当前 INS 任务频率 = 1 ms)
 *  1. 读 IMU yaw  → 更新当前航向
 *  2. 读编码器    → 航位推算, 累加位置
 *  3. 若在返航态  → PID 计算 vx/wz, 发布到底盘
 */
void INS_Task(void);

/* 启动返航: 清 PID 积分, 设置目标为原点 (0,0), 进入 RETURNING 状态 */
void INS_StartReturn(void);

/* 重置原点: 把当前位置记为新原点, 清空航位推算累积误差 */
void INS_ResetOrigin(void);

/* 读取当前坐标 (调试用) */
Position_t INS_GetPosition(void);

/* 读取当前状态 (调试用) */
INS_State_e INS_GetState(void);

/* 检查是否到达原点 */
NavReachState_e INS_Check_Target_Reach(void);

#endif
