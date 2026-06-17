#ifndef ROBOT_CMD_H
#define ROBOT_CMD_H


/**
 * @brief 机器人核心控制任务初始化模板,会被RobotInit()调用
 *
 */
void RobotCMDInit(void);

/**
 * @brief 机器人核心控制任务模板,默认由RobotTask()周期调用
 *
 */
void RobotCMDTask(void);

#endif // !ROBOT_CMD_H
