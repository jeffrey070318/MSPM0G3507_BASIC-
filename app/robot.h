#ifndef ROBOT_H
#define ROBOT_H

#include <stdbool.h>
#include <stdint.h>

extern volatile bool robot_oled_initialized;
extern volatile uint32_t robot_oled_refresh_count;
extern volatile uint32_t robot_oled_error_count;

/* Robot利用robot_def.h中的宏对不同的机器人进行了大量的兼容,同时兼容了两个开发板(云台板和底盘板)的配置 */

/**
 * @brief 机器人初始化,请在开启rtos之前调用.这也是唯一需要放入main函数的函数
 * 
 */
void RobotInit();

/**
 * @brief 机器人任务,放入实时系统以一定频率运行,内部会调用各个应用的任务
 * 
 */
void RobotTask();

/** 初始化（必要时重试）并刷新一帧整机调试参数。 */
void RobotOLEDTask(void);

#endif
