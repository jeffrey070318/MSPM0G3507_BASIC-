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

#include "bsp_usart.h"
#include "imu.h"
#include "vofa.h"

#include "ti_msp_dl_config.h"

static IMU_Handle_t imu_handle;
static USARTInstance *vofa_usart;

void RobotInit()
{  
    __disable_irq();
    
    BSPInit();

    {
        USART_Init_Config_s uart_config = {
            .recv_buff_size = 16U,
            .usart_handle = &huart1,
            .module_callback = NULL,
        };
        vofa_usart = USARTRegister(&uart_config);
    }

    {
        IIC_Init_Config_s iic_config = {
            .handle = &hi2c2,
            .dev_address = JY901S_I2C_ADDR,
            .work_mode = IIC_BLOCK_MODE,
            .callback = NULL,
            .id = NULL,
        };
        IMU_Init(&imu_handle, &iic_config);
    }

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
        if (IMU_ReadAll(&imu_handle, &imu_data) == DEVICE_OK) {
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
            vofa_justfloat_output_dma(vofa_buf, 9U, &huart1);
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
