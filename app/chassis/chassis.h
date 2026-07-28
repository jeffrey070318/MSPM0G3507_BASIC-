#ifndef CHASSIS_H
#define CHASSIS_H

#include <stdbool.h>

extern volatile bool chassis_manual_enabled;
extern volatile float chassis_manual_vx_mps;
extern volatile float chassis_manual_wz_radps;

/**
 * @brief 底盘应用初始化模板,请在开启rtos之前调用(目前会被RobotInit()调用)
 *
 */
void ChassisInit(void);

/**
 * @brief 底盘应用任务模板,放入实时系统以一定频率运行
 *
 */
void ChassisTask(void);

/** Enable the temporary manual command source used for chassis bring-up. */
void ChassisSetManualCommand(float vx_mps, float wz_radps);

/** Disable manual control and return the chassis to zero force. */
void ChassisDisableManualCommand(void);

#endif // CHASSIS_H
