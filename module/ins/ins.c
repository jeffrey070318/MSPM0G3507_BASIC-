/**
 * @file    ins.c
 * @brief   惯导/返航模块
 *
 *  核心数学（两个公式）:
 *    航位推算:  x += ds * cos(θ),  y += ds * sin(θ)
 *    返航控制:  dist = √(x²+y²),  err = atan2(-y,-x) - θ
 *              vx = PID_dist(dist) * cos(err),  wz = PID_angle(err)
 *
 *  数据流:
 *    JY901S → RobotTask → "imu_data" ──┐
 *                                      ├→ INS_Task → "chassis_cmd" → ChassisTask → 电机
 *    Encoder → ChassisTask → "encoder_data" ─┘
 */

#include "ins.h"
#include "message_center.h"
#include "bsp_log.h"
#include <string.h>

/* ================================================================
 *                    内部全局状态
 * ================================================================ */

static Subscriber_t  *g_imu_sub;      /* 订阅 "imu_data" */
static Subscriber_t  *g_enc_sub;      /* 订阅 "encoder_data" */
static Publisher_t   *g_cmd_pub;      /* 发布 "chassis_cmd" */

static Position_t     g_pos;          /* 当前坐标 (m), 原点 = 上电位置 */
static INS_State_e    g_state;        /* 状态机: IDLE / RETURNING / DONE */
static float          g_yaw;          /* JY901S 最新 yaw 角 (deg) */
static float          g_yaw0;         /* 原点时的 yaw 角 (deg), 用于计算相对航向 */
static int32_t        g_enc_l;        /* 左轮编码器上次数值 */
static int32_t        g_enc_r;        /* 右轮编码器上次数值 */
static bool           g_ok;           /* 初始化成功标志 */

static PID_Device_t   g_pid_dist;     /* 距离环 PID */
static PID_Device_t   g_pid_ang;      /* 角度环 PID */

/* ================================================================
 *                    内联数学库（替代 libm, 省 ROM）
 * ================================================================ */

/* 绝对值 */
static float AbsF(float v) { return (v < 0.0f) ? -v : v; }

/* 平方根 —— 牛顿迭代法, 8 次迭代精度 ~1e-6 */
static float SqrtF(float x) {
    if (x <= 0.0f) return 0.0f;
    float g = x * 0.5f;
    for (int i = 0; i < 8; i++) g = (g + x / g) * 0.5f;
    return g;
}

/* 正弦 —— 5 阶泰勒展开: sin(x) ≈ x - x³/3! + x⁵/5! - x⁷/7! */
static float SinF(float r) {
    float x2 = r * r;
    return r * (1.0f - x2*(0.16666667f - x2*(0.00833333f - x2*0.00019841f)));
}

/* 余弦 —— sin(x + π/2) */
static float CosF(float r)  { return SinF(r + 1.57079633f); }

/* atan2 —— 多项式近似, 返回值 [-π, π] */
static float Atan2F(float y, float x) {
    if (x == 0.0f)
        return (y > 0.0f) ? 1.57079633f : -1.57079633f;

    float z = y / x, az = AbsF(z), a;
    if (az <= 1.0f) {
        float z2 = z * z;
        a = z * (1.0f - z2*(0.33333333f - z2*(0.2f - z2*0.14285714f)));
    } else {
        float iz   = 1.0f / az;
        float iz2  = iz * iz;
        a = 1.57079633f - iz * (1.0f - iz2*(0.33333333f - iz2*(0.2f - iz2*0.14285714f)));
        if (z < 0.0f) a = -a;
    }
    return (x < 0.0f) ? ((y >= 0.0f) ? a + MY_PI : a - MY_PI) : a;
}

/* 角度归一化到 [-180°, 180°] */
static float Norm180(float d) {
    while (d > 180.0f)  d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    return d;
}

/* ================================================================
 *                    API 实现
 * ================================================================ */

/**
 * @brief  初始化 INS 模块
 *
 *  订阅: "imu_data"      (IMU_Data_t, 由 robot.c 每 5ms 发布)
 *  订阅: "encoder_data"  (Encoder_Pub_Data_t, 由 chassis.c 每 5ms 发布)
 *  注册: "chassis_cmd"   (Chassis_Cmd_Pub_t, 由本模块发布)
 *  初始化距离环 PID 和角度环 PID
 */
void INS_Init(void)
{
    memset(&g_pos, 0, sizeof(g_pos));
    g_state = INS_STATE_IDLE;
    g_yaw = g_yaw0 = 0.0f;
    g_enc_l = g_enc_r = 0;
    g_ok = false;

    g_imu_sub = SubRegister("imu_data", sizeof(IMU_Data_t));
    g_enc_sub = SubRegister(INS_ENCODER_TOPIC, sizeof(Encoder_Pub_Data_t));
    g_cmd_pub = PubRegister(INS_CMD_TOPIC, sizeof(Chassis_Cmd_Pub_t));

    if (!g_imu_sub || !g_enc_sub || !g_cmd_pub) {
        LOGERROR("[INS] Register failed");
        return;
    }

    PID_Init(&g_pid_dist, PID_POSITION,
             INS_DIST_KP, INS_DIST_KI, INS_DIST_KD,
             INS_DIST_MAX_OUT, INS_DIST_MAX_IOUT);

    PID_Init(&g_pid_ang,  PID_POSITION,
             INS_ANGLE_KP, INS_ANGLE_KI, INS_ANGLE_KD,
             INS_ANGLE_MAX_OUT, INS_ANGLE_MAX_IOUT);

    g_ok = true;
    LOGINFO("[INS] Init OK");
}

/**
 * @brief  INS 主任务 (1ms 周期)
 *
 *  三步:
 *   1. 读取 JY901S yaw → 更新当前航向 g_yaw
 *      首次读取时自动记录 g_yaw0 作为原点朝向
 *
 *   2. 读取编码器脉冲 → 航位推算更新 g_pos:
 *      ds = (Δ左 + Δ右) / 2 * 每脉冲米数     ← 两轮中心位移
 *      θ  = (g_yaw - g_yaw0) 转弧度          ← 相对航向
 *      x += ds * cos(θ)
 *      y += ds * sin(θ)
 *
 *   3. 若状态 = RETURNING, 执行返航 PID:
 *      dist    = √(x² + y²)                             ← 到原点距离
 *      tgt_deg = atan2(-y, -x) 转角度                   ← 原点方向
 *      err_deg = tgt_deg - 当前航向 (归一化到 ±180°)     ← 车头偏差
 *
 *      wz = 角度 PID(err_deg)                            ← 角速度
 *      vx = 距离 PID(dist) * cos(err_deg_rad)            ← 线速度
 *
 *      cos(err) 的作用: 车头没对准时减速, 先转再冲
 *
 *   最终发布 Chassis_Cmd_Pub_t 到 "chassis_cmd", 底盘据此控制电机
 */
void INS_Task(void)
{
    if (!g_ok) return;

    /* ======== 第 1 步: 读 IMU yaw ======== */
    IMU_Data_t imu;
    if (SubGetMessage(g_imu_sub, &imu)) {
        g_yaw = imu.yaw;                                    /* JY901S 九轴融合 yaw */
        /* 首次读数: 把当前朝向记作原点朝向 */
        if (g_yaw0 == 0.0f && g_pos.x == 0.0f && g_pos.y == 0.0f)
            g_yaw0 = imu.yaw;
    }

    /* ======== 第 2 步: 读编码器, 航位推算 ======== */
    Encoder_Pub_Data_t e;
    if (SubGetMessage(g_enc_sub, &e)) {
        int32_t dl = e.left_total  - g_enc_l;               /* 左轮本周期脉冲增量 */
        int32_t dr = e.right_total - g_enc_r;               /* 右轮本周期脉冲增量 */
        g_enc_l = e.left_total;
        g_enc_r = e.right_total;

        float ds = (float)(dl + dr) * 0.5f * INS_METERS_PER_COUNT;  /* 两轮中心位移 (m) */
        float r  = Norm180(g_yaw - g_yaw0) * DEG_TO_RAD;            /* 相对航向 (rad) */

        g_pos.x += ds * CosF(r);                            /* 公式 1: 航位推算 */
        g_pos.y += ds * SinF(r);
    }

    /* ======== 第 3 步: 返航 PID 控制 ======== */
    Chassis_Cmd_Pub_t cmd;
    memset(&cmd, 0, sizeof(cmd));

    if (g_state == INS_STATE_RETURNING) {
        float dx = -g_pos.x;                                /* 原点在 (0,0) */
        float dy = -g_pos.y;
        float dist = SqrtF(dx * dx + dy * dy);              /* 直线距离 */

        /* 到达阈值 → 停车 */
        if (dist < TARGET_DISTANCE) {
            g_state = INS_STATE_DONE;
            LOGINFO("[INS] Arrived dist=%.3f", (double)dist);
        } else {
            /* 目标方位角 ← atan2(-y, -x) */
            float tgt_deg = Atan2F(dy, dx) * (180.0f / MY_PI);
            float err_deg = Norm180(tgt_deg - Norm180(g_yaw - g_yaw0));

            /* 公式 2: 双层 PID */
            float wz = PID_Calc(&g_pid_ang,  0.0f, err_deg);    /* 角度环 → 角速度 */
            float vx = PID_Calc(&g_pid_dist, 0.0f, dist);       /* 距离环 → 线速度 */
            float c  = CosF(err_deg * DEG_TO_RAD);               /* cos(偏差角) */
            if (c < 0.0f) c = 0.0f;                              /* 偏差 > 90° 不冲 */
            vx *= c;                                             /* 对准了才加速 */

            cmd.vx = vx;
            cmd.wz = wz;
            cmd.chassis_mode = 1U;                               /* 1 = 运动模式 */
        }
    }
    /* IDLE / DONE 状态: cmd 保持全零 → 底盘卸力 */

    PubPushMessage(g_cmd_pub, &cmd);
}

/* ================================================================
 *                    返航控制
 * ================================================================ */

/**
 * @brief  启动返航: 清 PID 积分 → 目标设为原点 → 进入 RETURNING
 *
 *  假设原点 = (0, 0), 即上电位置
 */
void INS_StartReturn(void)
{
    if (!g_ok) return;

    PID_Clear(&g_pid_dist);                        /* 清积分, 否则旧误差累积 */
    PID_Clear(&g_pid_ang);
    g_state = INS_STATE_RETURNING;
    LOGINFO("[INS] Return (%.3f,%.3f)", (double)g_pos.x, (double)g_pos.y);
}

/**
 * @brief  重置原点: 把当前航向和位置记为新原点
 *
 *  用途: 消除长时间航位推算累积误差, 或者更换出发点
 */
void INS_ResetOrigin(void)
{
    if (!g_ok) return;

    g_yaw0    = g_yaw;                             /* 新原点航向 = 当前航向 */
    g_pos.x   = 0.0f;                              /* 新原点坐标 = (0,0) */
    g_pos.y   = 0.0f;
    g_enc_l   = 0;                                 /* 编码器参考值清零 */
    g_enc_r   = 0;
    g_state   = INS_STATE_IDLE;

    PID_Clear(&g_pid_dist);
    PID_Clear(&g_pid_ang);
    LOGINFO("[INS] Origin reset");
}

/* ================================================================
 *                    调试 / 查询
 * ================================================================ */

Position_t INS_GetPosition(void) { return g_pos; }
INS_State_e INS_GetState(void)   { return g_state; }

/**
 * @brief  检查是否到达原点
 *
 * @retval NAV_TARGET_REACHED     距离 < 5cm
 * @retval NAV_TARGET_APPROACHING 距离 < 30cm
 * @retval NAV_TARGET_FAR         距离 >= 30cm
 */
NavReachState_e INS_Check_Target_Reach(void)
{
    float d = SqrtF(g_pos.x * g_pos.x + g_pos.y * g_pos.y);
    if (d < TARGET_DISTANCE)          return NAV_TARGET_REACHED;
    if (d < TARGET_DISTANCE * 6.0f)   return NAV_TARGET_APPROACHING;   /* 6×5cm = 30cm */
    return NAV_TARGET_FAR;
}
