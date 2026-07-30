#include "pipe_axis.h"

#include <limits.h>

#define PIPE_AXIS_PI      3.14159265359f
#define PIPE_AXIS_HALF_PI 1.57079632679f
#define PIPE_AXIS_TWO_PI  6.28318530718f

/* 返回绝对值，用于判断灵敏度是否接近死点。 */
static float PipeAxisAbs(float value)
{
    return (value < 0.0f) ? -value : value;
}

/* 将角度折叠到 -pi~pi，减小三角函数近似的误差。 */
static float PipeAxisWrapPi(float angle)
{
    while (angle > PIPE_AXIS_PI) {
        angle -= PIPE_AXIS_TWO_PI;
    }
    while (angle < -PIPE_AXIS_PI) {
        angle += PIPE_AXIS_TWO_PI;
    }
    return angle;
}

/* 使用五阶泰勒近似计算 sin，避免在嵌入式链接 libm。 */
static float PipeAxisSin(float angle)
{
    float x = PipeAxisWrapPi(angle);
    if (x > PIPE_AXIS_HALF_PI) {
        x = PIPE_AXIS_PI - x;
    } else if (x < -PIPE_AXIS_HALF_PI) {
        x = -PIPE_AXIS_PI - x;
    }

    float x2 = x * x;
    return x * (1.0f - (x2 / 6.0f) + (x2 * x2 / 120.0f));
}

/* cos(x) = sin(x + pi/2)，复用同一套轻量近似。 */
static float PipeAxisCos(float angle)
{
    return PipeAxisSin(angle + PIPE_AXIS_HALF_PI);
}

/* 牛顿迭代求平方根，用于由 L2^2-Ax^2 计算连杆竖直投影。 */
static float PipeAxisSqrt(float value)
{
    if (value <= 0.0f) {
        return 0.0f;
    }

    float x = (value > 1.0f) ? value : 1.0f;
    for (uint8_t i = 0U; i < 8U; ++i) {
        x = 0.5f * (x + value / x);
    }
    return x;
}

/* 生成默认曲柄连杆参数：P 点位于 A 点上方，细分为 6400 pulse/rev。 */
static PipeAxis_Config_t PipeAxisDefaultConfig(void)
{
    PipeAxis_Config_t config = {
        .crank_mm = PIPE_AXIS_CRANK_MM,
        .rod_mm = PIPE_AXIS_ROD_MM,
        .init_theta_deg = PIPE_AXIS_INIT_THETA_DEG,
        .pulses_per_rev = PIPE_AXIS_PULSES_PER_REV,
        .min_abs_dy_per_rad = PIPE_AXIS_MIN_ABS_DY_PER_RAD,
        .reverse_motor_direction = false,
        .end_point_below_joint = false,
    };
    return config;
}

/* 初始化曲柄-连杆-竖直滑块模型，并检查几何参数是否可解。 */
Device_Status_e PipeAxis_Init(
    PipeAxis_t *axis, const PipeAxis_Config_t *config)
{
    if (axis == NULL) {
        return DEVICE_ERROR;
    }

    axis->config = (config != NULL) ? *config : PipeAxisDefaultConfig();
    if ((axis->config.crank_mm <= 0.0f) ||
        (axis->config.rod_mm <= axis->config.crank_mm) ||
        (axis->config.pulses_per_rev <= 0.0f) ||
        (axis->config.min_abs_dy_per_rad < 0.0f)) {
        axis->initialized = false;
        return DEVICE_ERROR;
    }

    axis->position_pulses = 0;
    axis->initialized = true;
    return DEVICE_OK;
}

/* 用初始角和累计脉冲估算当前曲柄角 theta。 */
float PipeAxis_GetThetaRad(const PipeAxis_t *axis)
{
    if ((axis == NULL) || !axis->initialized) {
        return 0.0f;
    }

    float init_theta =
        axis->config.init_theta_deg * PIPE_AXIS_PI / 180.0f;
    float motor_theta =
        ((float) axis->position_pulses / axis->config.pulses_per_rev) *
        PIPE_AXIS_TWO_PI;
    if (axis->config.reverse_motor_direction) {
        motor_theta = -motor_theta;
    }
    return init_theta + motor_theta;
}

/* 由 theta 计算 A 点横坐标：Ax = L1*cos(theta)。 */
float PipeAxis_GetJointXmm(const PipeAxis_t *axis)
{
    if ((axis == NULL) || !axis->initialized) {
        return 0.0f;
    }

    return axis->config.crank_mm * PipeAxisCos(PipeAxis_GetThetaRad(axis));
}

/* 由 theta 计算 A 点纵坐标：Ay = L1*sin(theta)。 */
float PipeAxis_GetJointYmm(const PipeAxis_t *axis)
{
    if ((axis == NULL) || !axis->initialized) {
        return 0.0f;
    }

    return axis->config.crank_mm * PipeAxisSin(PipeAxis_GetThetaRad(axis));
}

/* 根据 AP=L2 且 P.x=0，计算 P 相对 A 的竖直偏移量。 */
static Device_Status_e PipeAxisGetRodVerticalOffset(
    const PipeAxis_t *axis, float *offset)
{
    float joint_x = PipeAxis_GetJointXmm(axis);
    float radicand =
        axis->config.rod_mm * axis->config.rod_mm - joint_x * joint_x;
    if (radicand < 0.0f) {
        *offset = 0.0f;
        return DEVICE_ERROR;
    }

    *offset = PipeAxisSqrt(radicand);
    if (axis->config.end_point_below_joint) {
        *offset = -*offset;
    }
    return DEVICE_OK;
}

/* 计算 P 点高度：Py = Ay +/- sqrt(L2^2 - Ax^2)。 */
float PipeAxis_GetEndYmm(const PipeAxis_t *axis)
{
    if ((axis == NULL) || !axis->initialized) {
        return 0.0f;
    }

    float offset = 0.0f;
    if (PipeAxisGetRodVerticalOffset(axis, &offset) != DEVICE_OK) {
        return 0.0f;
    }
    return PipeAxis_GetJointYmm(axis) + offset;
}

/* 计算 dPy/dtheta，用于把期望竖直位移反解成电机角度。 */
static Device_Status_e PipeAxisGetDyPerRad(
    const PipeAxis_t *axis, float *dy_per_rad)
{
    float theta = PipeAxis_GetThetaRad(axis);
    float sin_theta = PipeAxisSin(theta);
    float cos_theta = PipeAxisCos(theta);
    float joint_x = axis->config.crank_mm * cos_theta;
    float radicand =
        axis->config.rod_mm * axis->config.rod_mm - joint_x * joint_x;
    if (radicand <= 0.0f) {
        *dy_per_rad = 0.0f;
        return DEVICE_ERROR;
    }

    float rod_vertical = PipeAxisSqrt(radicand);
    float branch = axis->config.end_point_below_joint ? -1.0f : 1.0f;
    *dy_per_rad = axis->config.crank_mm * cos_theta +
                  branch * axis->config.crank_mm * axis->config.crank_mm *
                      sin_theta * cos_theta / rod_vertical;
    return DEVICE_OK;
}

/* 将 P 点微动 delta_y_mm 反解为脉冲，死点附近会返回 DEVICE_ERROR。 */
Device_Status_e PipeAxis_DeltaMmToPulses(
    PipeAxis_t *axis, float delta_y_mm, int32_t *pulses)
{
    if ((axis == NULL) || !axis->initialized || (pulses == NULL)) {
        return DEVICE_ERROR;
    }

    float dy_per_rad = 0.0f;
    if ((PipeAxisGetDyPerRad(axis, &dy_per_rad) != DEVICE_OK) ||
        (PipeAxisAbs(dy_per_rad) < axis->config.min_abs_dy_per_rad)) {
        *pulses = 0;
        return DEVICE_ERROR;
    }

    float delta_theta = delta_y_mm / dy_per_rad;
    if (axis->config.reverse_motor_direction) {
        delta_theta = -delta_theta;
    }

    float pulse_float =
        delta_theta / PIPE_AXIS_TWO_PI * axis->config.pulses_per_rev;
    if (pulse_float > (float) INT32_MAX) {
        *pulses = INT32_MAX;
        return DEVICE_ERROR;
    }
    if (pulse_float < (float) INT32_MIN) {
        *pulses = INT32_MIN;
        return DEVICE_ERROR;
    }

    *pulses = (int32_t) pulse_float;
    return DEVICE_OK;
}

/* 在步进电机完成相对移动后，累加脉冲位置以保持 theta 估计同步。 */
void PipeAxis_ApplyPulseFeedback(PipeAxis_t *axis, int32_t pulses)
{
    if ((axis == NULL) || !axis->initialized) {
        return;
    }

    axis->position_pulses += pulses;
}

/* 机械归零或人工摆到初始姿态后，重置当前脉冲位置。 */
void PipeAxis_ResetPosition(PipeAxis_t *axis, int32_t position_pulses)
{
    if (axis == NULL) {
        return;
    }

    axis->position_pulses = position_pulses;
}
