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
// #define ROBOT_ENABLE_CHASSIS_APP
// #define ROBOT_ENABLE_GIMBAL_APP
// #define ROBOT_ENABLE_SHOOT_APP

/* --------------------------视觉链路模板-------------------------- */
// #define VISION_USE_VCP
// #define VISION_USE_UART

#if defined(VISION_USE_VCP) && defined(VISION_USE_UART)
#error Conflict vision definition! Choose VCP or UART.
#endif

/* --------------------------机器人参数模板-------------------------- */
/* 以下宏名保留给旧框架代码引用，默认值不代表实际机器人参数。 */
#define YAW_CHASSIS_ALIGN_ECD (0U)
#define YAW_ECD_GREATER_THAN_4096 (0)
#define PITCH_HORIZON_ECD (0U)
#define PITCH_MAX_ANGLE (0.0f)
#define PITCH_MIN_ANGLE (0.0f)

#define ONE_BULLET_DELTA_ANGLE (0.0f)
#define REDUCTION_RATIO_LOADER (1.0f)
#define NUM_PER_CIRCLE (1U)

#define WHEEL_BASE (0.0f)
#define TRACK_WIDTH (0.0f)
#define CENTER_GIMBAL_OFFSET_X (0.0f)
#define CENTER_GIMBAL_OFFSET_Y (0.0f)
#define RADIUS_WHEEL (0.0f)
#define REDUCTION_RATIO_WHEEL (1.0f)

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
