#ifndef CHASSIS_H
#define CHASSIS_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_def.h"

typedef enum {
    CHASSIS_LINE_TRACE_IDLE = 0,
    CHASSIS_LINE_TRACE_RUNNING,
    CHASSIS_LINE_TRACE_DONE,
    CHASSIS_LINE_TRACE_ERROR,
} Chassis_LineTrace_State_e;

extern volatile bool chassis_manual_enabled;
extern volatile float chassis_manual_vx_mps;
extern volatile float chassis_manual_wz_radps;
extern volatile Device_Status_e chassis_line_trace_init_status;
extern volatile Chassis_LineTrace_State_e chassis_line_trace_state;
extern volatile uint32_t chassis_line_trace_key1_press_count;
extern volatile uint32_t chassis_line_trace_key3_press_count;
extern volatile uint32_t chassis_line_trace_cross_count;
extern volatile uint32_t chassis_line_trace_raw;
extern volatile uint32_t chassis_line_trace_active_count;
extern volatile float chassis_line_trace_offset;
extern volatile float chassis_line_trace_base_vx_mps;
extern volatile float chassis_line_trace_turn_kp;
extern volatile uint32_t chassis_line_trace_stop_active_threshold;
extern volatile uint32_t chassis_line_trace_key_active_level;
extern volatile uint32_t chassis_line_trace_gray_active_level;

/** 初始化两路底盘电机；由 RobotInit() 在调度器启动前调用。 */
void ChassisInit(void);

/** 接收命令并更新差速运动学和两路速度闭环。 */
void ChassisTask(void);

/** 启用手动联调命令，速度单位为 m/s 和 rad/s。 */
void ChassisSetManualCommand(float vx_mps, float wz_radps);

/** 禁用手动联调命令并恢复零力状态。 */
void ChassisDisableManualCommand(void);

#endif // CHASSIS_H
