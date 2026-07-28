#ifndef CHASSIS_H
#define CHASSIS_H

#include <stdbool.h>

extern volatile bool chassis_manual_enabled;
extern volatile float chassis_manual_vx_mps;
extern volatile float chassis_manual_wz_radps;

/** 初始化两路底盘电机；由 RobotInit() 在调度器启动前调用。 */
void ChassisInit(void);

/** 接收命令并更新差速运动学和两路速度闭环。 */
void ChassisTask(void);

/** 启用手动联调命令，速度单位为 m/s 和 rad/s。 */
void ChassisSetManualCommand(float vx_mps, float wz_radps);

/** 禁用手动联调命令并恢复零力状态。 */
void ChassisDisableManualCommand(void);

#endif // CHASSIS_H
