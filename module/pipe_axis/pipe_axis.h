#ifndef MODULE_PIPE_AXIS_H
#define MODULE_PIPE_AXIS_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PIPE_AXIS_CRANK_MM            35.0f
#define PIPE_AXIS_ROD_MM              55.5f
#define PIPE_AXIS_INIT_THETA_DEG      90.0f
#define PIPE_AXIS_PULSES_PER_REV      6400.0f
#define PIPE_AXIS_MIN_ABS_DY_PER_RAD  2.0f

typedef struct {
    float crank_mm;
    float rod_mm;
    float init_theta_deg;
    float pulses_per_rev;
    float min_abs_dy_per_rad;
    bool reverse_motor_direction;
    bool end_point_below_joint;
} PipeAxis_Config_t;

typedef struct {
    PipeAxis_Config_t config;
    int32_t position_pulses;
    bool initialized;
} PipeAxis_t;

/* 初始化水管微动轴模型；config 为空时使用本文件上方的默认机构参数。 */
Device_Status_e PipeAxis_Init(
    PipeAxis_t *axis, const PipeAxis_Config_t *config);
/* 根据当前位置脉冲，计算第一根曲柄相对零位的当前角度，单位 rad。 */
float PipeAxis_GetThetaRad(const PipeAxis_t *axis);
/* 计算两杆连接点 A 的水平坐标，O/P 竖直线为 x=0，单位 mm。 */
float PipeAxis_GetJointXmm(const PipeAxis_t *axis);
/* 计算两杆连接点 A 的竖直坐标，单位 mm。 */
float PipeAxis_GetJointYmm(const PipeAxis_t *axis);
/* 计算接水管点 P 的竖直坐标，单位 mm。 */
float PipeAxis_GetEndYmm(const PipeAxis_t *axis);
/* 将 P 点期望竖直微动量 delta_y_mm 换算为电机相对脉冲数。 */
Device_Status_e PipeAxis_DeltaMmToPulses(
    PipeAxis_t *axis, float delta_y_mm, int32_t *pulses);
/* 电机实际执行完脉冲后，用它更新机构内部角度估计。 */
void PipeAxis_ApplyPulseFeedback(PipeAxis_t *axis, int32_t pulses);
/* 上电归零或人工标定后，重置机构内部脉冲位置。 */
void PipeAxis_ResetPosition(PipeAxis_t *axis, int32_t position_pulses);

#ifdef __cplusplus
}
#endif

#endif
