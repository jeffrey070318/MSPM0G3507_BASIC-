/**
 * @file robot_def.h
 * @brief 机器人公共定义模板
 *
 * 本文件只保留 app/module 层共享的宏名、枚举名、结构体名和字段名。
 * 具体机器人参数、视觉链路和 app 启用开关需要按实车重新配置。
 */
#pragma once
#ifndef ROBOT_DEF_H
#define ROBOT_DEF_H

#include <stdint.h>

#if __has_include("ins_task.h")
#include "ins_task.h"
#else
typedef struct {
    float yaw;
    float pitch;
    float roll;
} attitude_t;
#endif

#if __has_include("master_process.h")
#include "master_process.h"
#else
typedef enum {
    BULLET_SPEED_UNKNOWN = 0,
} Bullet_Speed_e;

typedef enum {
    ENEMY_BLUE = 0,
    ENEMY_RED = 1,
} Enemy_Color_e;
#endif

/* --------------------------应用启用开关模板-------------------------- */
/* 按迁移进度打开对应 app。打开前请确认源文件和依赖模块已经完成适配。 */
// #define ROBOT_ENABLE_CMD_APP
#define ROBOT_ENABLE_CHASSIS_APP
// #define ROBOT_ENABLE_INS_APP
// #define ROBOT_ENABLE_GIMBAL_APP
// #define ROBOT_ENABLE_SHOOT_APP

#if defined(ROBOT_ENABLE_INS_APP) && !defined(ROBOT_ENABLE_CHASSIS_APP)
#error ROBOT_ENABLE_INS_APP requires ROBOT_ENABLE_CHASSIS_APP.
#endif

/* --------------------------视觉链路模板-------------------------- */
// #define VISION_USE_VCP
// #define VISION_USE_UART

#if defined(VISION_USE_VCP) && defined(VISION_USE_UART)
#error Conflict vision definition! Choose VCP or UART.
#endif

/* ==========================实车调参区域========================== */

/* --------------------------循迹参数------------------------------ */
/* 以下为低速初值，灰度方向、速度和 PID 均需实车验证。 */
#define LINE_FOLLOW_SENSOR_ACTIVE_STATE  GPIO_PIN_SET
#define LINE_FOLLOW_SENSOR_CHANNEL_ORDER GRAY_SENSOR_CHANNEL_1_ON_LEFT
#define LINE_FOLLOW_SENSOR_SETTLE_US     5U
#define LINE_FOLLOW_BASE_SPEED_MPS       0.15f
#define LINE_FOLLOW_MAX_WZ_RADPS         1.5f
#define LINE_FOLLOW_KP                   2.0f
#define LINE_FOLLOW_KI                   0.0f
#define LINE_FOLLOW_KD                   0.0f
#define LINE_FOLLOW_MAX_IOUT             0.5f
#define LINE_FOLLOW_DEADBAND             0.02f
#define LINE_FOLLOW_A_MARKER_ACTIVE_MIN       6U
#define LINE_FOLLOW_A_MARKER_DEBOUNCE_SAMPLES 3U
#define LINE_FOLLOW_A_MARKER_REARM_SAMPLES    3U

/* --------------------------平衡机构参数-------------------------- */
/* UART3 暂分配给视觉；协议确定前平衡任务不会发出运动命令。 */
#define BALL_BALANCE_VISION_PORT TRANSPARENT_UART_PORT_3
#define BALL_BALANCE_VISION_TIMEOUT_MS 100U
#define BALL_BALANCE_TARGET_POSITION   0.0f
#define BALL_BALANCE_SOFT_LIMIT_STEPS  2000
#define BALL_BALANCE_MAX_MOVE_STEPS    20U
#define BALL_BALANCE_STEPPER_SPEED_SPS 200U

/* --------------------------底盘机械参数-------------------------- */
/* 车轮有效半径，单位 m；应以车辆实际行驶距离反算校准。 */
#define CHASSIS_WHEEL_RADIUS_M (0.076f)
/* 左右驱动轮接地点中心距，单位 m；影响差速转向角速度。 */
#define CHASSIS_TRACK_WIDTH_M  (0.40f)

/* ------------------------编码器与传动参数------------------------ */
/* 单个编码器通道的电机轴每圈脉冲数，不包含 AB 相倍频。 */
#define CHASSIS_ENCODER_PPR        (11.0f)
/* 当前四状态正交解码为 4 倍频。 */
#define CHASSIS_ENCODER_QUADRATURE (4.0f)
/* 电机轴转数 / 车轮轴转数。 */
#define CHASSIS_MOTOR_GEAR_RATIO   (19.0f)

/* --------------------------速度环参数---------------------------- */
/* 目标与反馈单位均为 encoder counts/s，输出范围为 -1.0 ~ 1.0。 */
#define CHASSIS_SPEED_KP       (0.000025f)
#define CHASSIS_SPEED_KI       (0.0002f)
#define CHASSIS_SPEED_KD       (0.0f)
#define CHASSIS_SPEED_MAX_OUT  (1.0f)
#define CHASSIS_SPEED_MAX_IOUT (0.5f)

/* --------------------------电机方向参数-------------------------- */
/* 只能填 0 或 1；同一个值会同时反转驱动方向和编码器反馈方向。 */
#define CHASSIS_LEFT_MOTOR_REVERSE  (0)
#define CHASSIS_RIGHT_MOTOR_REVERSE (0)

#if ((CHASSIS_LEFT_MOTOR_REVERSE != 0) && \
        (CHASSIS_LEFT_MOTOR_REVERSE != 1)) || \
    ((CHASSIS_RIGHT_MOTOR_REVERSE != 0) && \
        (CHASSIS_RIGHT_MOTOR_REVERSE != 1))
#error Chassis motor reverse parameters must be 0 or 1.
#endif

/* ========================旧框架兼容参数======================== */
/* 以下宏名保留给尚未完成适配的旧框架代码引用。 */

/* 云台零位与限位占位参数。 */
#define YAW_CHASSIS_ALIGN_ECD (0U)
#define YAW_ECD_GREATER_THAN_4096 (0)
#define PITCH_HORIZON_ECD (0U)
#define PITCH_MAX_ANGLE (0.0f)
#define PITCH_MIN_ANGLE (0.0f)

/* 发射机构占位参数。 */
#define ONE_BULLET_DELTA_ANGLE (0.0f)
#define REDUCTION_RATIO_LOADER (1.0f)
#define NUM_PER_CIRCLE (1U)

/* 旧底盘宏别名；轮距、轮径和减速比复用当前实车参数。 */
#define WHEEL_BASE (0.0f)
#define TRACK_WIDTH CHASSIS_TRACK_WIDTH_M
#define CENTER_GIMBAL_OFFSET_X (0.0f)
#define CENTER_GIMBAL_OFFSET_Y (0.0f)
#define RADIUS_WHEEL CHASSIS_WHEEL_RADIUS_M
#define REDUCTION_RATIO_WHEEL CHASSIS_MOTOR_GEAR_RATIO

/* IMU 坐标系到云台坐标系的方向占位参数。 */
#define GYRO2GIMBAL_DIR_YAW (1)
#define GYRO2GIMBAL_DIR_PITCH (1)
#define GYRO2GIMBAL_DIR_ROLL (1)

#pragma pack(1)

/* --------------------------基本状态类型-------------------------- */
typedef enum {
    ROBOT_STOP = 0,
    ROBOT_READY,
} Robot_Status_e;

typedef enum {
    APP_OFFLINE = 0,
    APP_ONLINE,
    APP_ERROR,
} App_Status_e;

typedef enum {
    CHASSIS_ZERO_FORCE = 0,
    CHASSIS_ROTATE,
    CHASSIS_NO_FOLLOW,
    CHASSIS_FOLLOW_GIMBAL_YAW,
} chassis_mode_e;

typedef enum {
    GIMBAL_ZERO_FORCE = 0,
    GIMBAL_FREE_MODE,
    GIMBAL_GYRO_MODE,
} gimbal_mode_e;

typedef enum {
    SHOOT_OFF = 0,
    SHOOT_ON,
} shoot_mode_e;

typedef enum {
    FRICTION_OFF = 0,
    FRICTION_ON,
} friction_mode_e;

typedef enum {
    LID_OPEN = 0,
    LID_CLOSE,
} lid_mode_e;

typedef enum {
    LOAD_STOP = 0,
    LOAD_REVERSE,
    LOAD_1_BULLET,
    LOAD_3_BULLET,
    LOAD_BURSTFIRE,
} loader_mode_e;

/* --------------------------共享控制/反馈数据-------------------------- */
typedef struct {
    float chassis_power_mx;
} Chassis_Power_Data_s;

typedef struct {
    float vx;
    float vy;
    float wz;
    float offset_angle;
    chassis_mode_e chassis_mode;
    int chassis_speed_buff;
} Chassis_Ctrl_Cmd_s;

typedef struct {
    float yaw;
    float pitch;
    float chassis_rotate_wz;
    gimbal_mode_e gimbal_mode;
} Gimbal_Ctrl_Cmd_s;

typedef struct {
    shoot_mode_e shoot_mode;
    loader_mode_e load_mode;
    lid_mode_e lid_mode;
    friction_mode_e friction_mode;
    Bullet_Speed_e bullet_speed;
    uint8_t rest_heat;
    float shoot_rate;
} Shoot_Ctrl_Cmd_s;

typedef struct {
    uint8_t rest_heat;
    Bullet_Speed_e bullet_speed;
    Enemy_Color_e enemy_color;
} Chassis_Upload_Data_s;

typedef struct {
    attitude_t gimbal_imu_data;
    uint16_t yaw_motor_single_round_angle;
} Gimbal_Upload_Data_s;

typedef struct {
    uint8_t reserved;
} Shoot_Upload_Data_s;

#pragma pack()

#endif
