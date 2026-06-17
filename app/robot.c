#include "bsp_init.h"
#include "robot.h"
#include "robot_def.h"
#include "robot_task.h"

#if defined(ROBOT_ENABLE_CHASSIS_APP)
#include "chassis.h"
#endif

#if defined(ROBOT_ENABLE_GIMBAL_APP)
#include "gimbal.h"
#endif

#if defined(ROBOT_ENABLE_SHOOT_APP)
#include "shoot.h"
#endif

#if defined(ROBOT_ENABLE_CMD_APP)
#include "robot_cmd.h"
#endif


void RobotInit()
{  
    // 关闭中断,防止在初始化过程中发生中断
    // 请不要在初始化过程中使用中断和延时函数！
    // 若必须,则只允许使用DWT_Delay()
    __disable_irq();
    
    BSPInit();

#if defined(ROBOT_ENABLE_CMD_APP)
    RobotCMDInit();
#endif

#if defined(ROBOT_ENABLE_GIMBAL_APP)
    // GimbalInit();
#endif

#if defined(ROBOT_ENABLE_SHOOT_APP)
    // ShootInit();
#endif

#if defined(ROBOT_ENABLE_CHASSIS_APP)
    ChassisInit();
#endif

    OSTaskInit(); // 创建基础任务

    // 初始化完成,开启中断
    __enable_irq();
}

void RobotTask()
{
#if defined(ROBOT_ENABLE_CMD_APP)
    RobotCMDTask();
#endif

#if defined(ROBOT_ENABLE_GIMBAL_APP)
    // GimbalTask();
#endif

#if defined(ROBOT_ENABLE_SHOOT_APP)
    // ShootTask();
#endif

#if defined(ROBOT_ENABLE_CHASSIS_APP)
    ChassisTask();
#endif

}
