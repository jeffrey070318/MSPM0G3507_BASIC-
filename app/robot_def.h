/**
 * @file robot_def.h
 * @brief H 题整车调参入口
 *
 * 参数按比赛、循迹、平衡机构和底盘分区。带物理量的宏名包含单位，
 * 设备句柄和运行状态不得放入本文件。
 */
#ifndef ROBOT_DEF_H
#define ROBOT_DEF_H

/* --------------------------比赛参数------------------------------ */
#define COMPETITION_KEY_DEBOUNCE_SAMPLES 3U
#define COMPETITION_TIME_LIMIT_MS 120000U
#define COMPETITION_STOP_A_MARKER_COUNT 2U

/* --------------------------循迹参数------------------------------ */
/* 以下为低速初值，灰度方向、速度和 PID 均需实车验证。 */
#define LINE_FOLLOW_SENSOR_ACTIVE_STATE GPIO_PIN_SET
#define LINE_FOLLOW_SENSOR_CHANNEL_ORDER GRAY_SENSOR_CHANNEL_1_ON_LEFT
#define LINE_FOLLOW_SENSOR_SETTLE_US 5U
#define LINE_FOLLOW_BASE_SPEED_MPS 0.30f
#define LINE_FOLLOW_MAX_WZ_RADPS 1.5f
#define LINE_FOLLOW_KP 2.0f
#define LINE_FOLLOW_KI 0.0f
#define LINE_FOLLOW_KD 0.0f
#define LINE_FOLLOW_MAX_IOUT 0.5f
#define LINE_FOLLOW_DEADBAND 0.02f
#define LINE_FOLLOW_A_MARKER_ACTIVE_MIN 6U
#define LINE_FOLLOW_A_MARKER_DEBOUNCE_SAMPLES 3U
#define LINE_FOLLOW_A_MARKER_REARM_SAMPLES 3U
#define LINE_FOLLOW_LOST_LINE_HOLD_SAMPLES 10U

/* KEY1 line-follow debug: stop after this run time. */
#define COMPETITION_KEY1_TIME_STOP_ENABLED 1
#define COMPETITION_KEY1_TIME_STOP_MS 19000U

/* --------------------------平衡机构参数-------------------------- */
/* UART3 remains reserved for vision; without a camera, use manual level bounce. */
#define BALL_BALANCE_VISION_PORT TRANSPARENT_UART_PORT_3
#define BALL_BALANCE_VISION_TIMEOUT_MS 100U
#define BALL_BALANCE_TARGET_POSITION 0.0f
#define BALL_BALANCE_SOFT_LIMIT_STEPS 0
#define BALL_BALANCE_MAX_MOVE_STEPS 20U
#define BALL_BALANCE_STEPPER_SPEED_SPS 200U
#define BALL_BALANCE_LEVEL_KEY_ACTIVE_STATE 0U
#define BALL_BALANCE_LEVEL_KEY_DEBOUNCE_SAMPLES 20U
/* Open-loop feedforward fallback before the camera/PID path is available. */
#define BALL_BALANCE_OPEN_LOOP_ENABLED 1
#define BALL_BALANCE_OPEN_LOOP_SPEED_SPS 800U
#define BALL_BALANCE_OPEN_POS_GO_TARGET_STEPS 2800
#define BALL_BALANCE_OPEN_POS_GO_HOLD_MS 800U
#define BALL_BALANCE_OPEN_POS_BRAKE_TARGET_STEPS -150
#define BALL_BALANCE_OPEN_POS_BRAKE_HOLD_MS 250U
#define BALL_BALANCE_OPEN_POS_SETTLE_TARGET_STEPS 0
#define BALL_BALANCE_OPEN_POS_SETTLE_HOLD_MS 300U
#define BALL_BALANCE_OPEN_NEG_GO_TARGET_STEPS -400
#define BALL_BALANCE_OPEN_NEG_GO_HOLD_MS 800U
#define BALL_BALANCE_OPEN_NEG_BRAKE_TARGET_STEPS 120
#define BALL_BALANCE_OPEN_NEG_BRAKE_HOLD_MS 250U
#define BALL_BALANCE_OPEN_FINAL_HOLD_TARGET_STEPS 0

/* --------------------------底盘机械参数-------------------------- */
/* 车轮有效半径和左右轮接地点中心距，单位 m。 */
#define CHASSIS_WHEEL_RADIUS_M 0.065f
#define CHASSIS_TRACK_WIDTH_M 0.225f

/* 单通道电机轴脉冲、AB 四倍频和电机轴/车轮轴减速比。 */
#define CHASSIS_ENCODER_PPR 11.0f
#define CHASSIS_ENCODER_QUADRATURE 4.0f
#define CHASSIS_MOTOR_GEAR_RATIO 21.6f

/* 速度环输入为 counts/s，输出为 -1.0 到 1.0。 */
#define CHASSIS_SPEED_KP 0.0007f
#define CHASSIS_SPEED_KI 0.0002f
#define CHASSIS_SPEED_KD 0.0f
#define CHASSIS_SPEED_MAX_OUT 1.0f
#define CHASSIS_SPEED_MAX_IOUT 0.5f

/* 临时实车开环验证；确认机械和驱动后必须恢复为 0。 */
#define CHASSIS_OPEN_LOOP_TEST_ENABLED 1
#define CHASSIS_OPEN_LOOP_TEST_OUTPUT 0.40f
#define CHASSIS_OPEN_LOOP_TEST_TURN_OUTPUT 0.20f
#define CHASSIS_OPEN_LOOP_TEST_TURN_WZ_FULL_SCALE_RADPS 1.5f

/* 驱动和编码器方向必须分别校准，使正速度指令得到正编码器反馈。 */
#define CHASSIS_LEFT_DRIVER_REVERSE 0
#define CHASSIS_LEFT_ENCODER_REVERSE 1
#define CHASSIS_RIGHT_DRIVER_REVERSE 1
#define CHASSIS_RIGHT_ENCODER_REVERSE 0

#if ((CHASSIS_LEFT_DRIVER_REVERSE != 0) &&   \
     (CHASSIS_LEFT_DRIVER_REVERSE != 1)) ||  \
    ((CHASSIS_LEFT_ENCODER_REVERSE != 0) &&  \
     (CHASSIS_LEFT_ENCODER_REVERSE != 1)) || \
    ((CHASSIS_RIGHT_DRIVER_REVERSE != 0) &&  \
     (CHASSIS_RIGHT_DRIVER_REVERSE != 1)) || \
    ((CHASSIS_RIGHT_ENCODER_REVERSE != 0) && \
     (CHASSIS_RIGHT_ENCODER_REVERSE != 1))
#error Chassis direction parameters must be 0 or 1.
#endif

#if ((CHASSIS_OPEN_LOOP_TEST_ENABLED != 0) && \
     (CHASSIS_OPEN_LOOP_TEST_ENABLED != 1))
#error Chassis open-loop test switch must be 0 or 1.
#endif

#endif
