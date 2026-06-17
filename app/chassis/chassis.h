#ifndef CHASSIS_H
#define CHASSIS_H

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

#endif // CHASSIS_H
