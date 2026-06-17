/**
 * @file chassis.c
 * @brief 底盘应用模板
 *
 * 底盘坐标系建议沿用原框架约定:
 * - x: 底盘正前方
 * - y: 底盘右侧
 * - wz: 俯视顺/逆时针旋转角速度,具体符号按实车电机安装方向确认
 *
 * 当前文件只保留 app 层流程骨架。具体电机类型、驱动方式、运动学模型、
 * 功率限制、裁判系统、消息发布/订阅等内容需要按实际底盘重新接入。
 */

#include "chassis.h"
#include "robot_def.h"

static Chassis_Ctrl_Cmd_s chassis_cmd_recv;
static Chassis_Upload_Data_s chassis_feedback_data;

/**
 * @brief 接收 robot_cmd 下发的底盘控制命令
 */
static void ChassisReceiveCommand(void)
{
    (void) &chassis_cmd_recv;
}

/**
 * @brief 根据控制命令更新底盘工作模式和目标速度
 */
static void ChassisUpdateMode(void)
{
    (void) &chassis_cmd_recv;
}

/**
 * @brief 按实际底盘结构完成运动学解算
 */
static void ChassisKinematicsSolve(void)
{
    (void) &chassis_cmd_recv;
}

/**
 * @brief 将解算结果输出到底盘电机或执行机构
 */
static void ChassisApplyOutput(void)
{
}

/**
 * @brief 采集底盘状态并更新反馈数据
 */
static void ChassisUpdateFeedback(void)
{
    (void) &chassis_feedback_data;
}

/**
 * @brief 发布底盘反馈给 robot_cmd 或 UI
 */
static void ChassisPublishFeedback(void)
{
    (void) &chassis_feedback_data;
}

/**
 * @brief 底盘应用初始化模板,请在开启 RTOS 之前调用
 */
void ChassisInit(void)
{
}

/**
 * @brief 底盘应用周期任务模板
 */
void ChassisTask(void)
{
    ChassisReceiveCommand();
    ChassisUpdateMode();
    ChassisKinematicsSolve();
    ChassisApplyOutput();
    ChassisUpdateFeedback();
    ChassisPublishFeedback();
}
