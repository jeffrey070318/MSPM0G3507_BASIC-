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

#include "imu.h"
#include "vofa.h"

#if defined(ROBOT_ENABLE_INS_APP)
#include "ins.h"
#include "message_center.h"

static Publisher_t *g_imu_publisher;
#endif

void RobotInit()
{  
    __disable_irq();
    
    BSPInit();

    (void) VOFA_Init();
    (void) IMU_Init();

#if defined(ROBOT_ENABLE_INS_APP)
    g_imu_publisher = PubRegister(INS_IMU_TOPIC, sizeof(IMU_Data_t));
#endif

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

    OSTaskInit();

    __enable_irq();
}

void RobotTask()
{
    {
        IMU_Data_t imu_data;
        if (IMU_ReadAll(&imu_data) == DEVICE_OK) {
            float vofa_buf[9];
            vofa_buf[0] = imu_data.ax;
            vofa_buf[1] = imu_data.ay;
            vofa_buf[2] = imu_data.az;
            vofa_buf[3] = imu_data.gx;
            vofa_buf[4] = imu_data.gy;
            vofa_buf[5] = imu_data.gz;
            vofa_buf[6] = imu_data.roll;
            vofa_buf[7] = imu_data.pitch;
            vofa_buf[8] = imu_data.yaw;
            (void) VOFA_JustFloatOutputDMA(vofa_buf, 9U);

#if defined(ROBOT_ENABLE_INS_APP)
            if (g_imu_publisher != NULL) {
                (void) PubPushMessage(g_imu_publisher, &imu_data);
            }
#endif
        }
    }

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
