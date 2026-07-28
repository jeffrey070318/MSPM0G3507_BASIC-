#include "bsp_init.h"
#include "robot.h"
#include "robot_def.h"
#include "robot_task.h"

#if defined(ROBOT_ENABLE_CHASSIS_APP)
#include "chassis.h"
#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
#include "motor.h"
#endif
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
#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
#include "oled.h"
#endif
#include "vofa.h"

#if defined(ROBOT_ENABLE_CHASSIS_APP) && \
    (HARDWARE_TEST_MODE == HARDWARE_TEST_NONE)
extern Motor_Device_t chassis_motors[2];
#endif

volatile bool robot_oled_initialized;
volatile uint32_t robot_oled_refresh_count;
volatile uint32_t robot_oled_error_count;

#if defined(ROBOT_ENABLE_INS_APP)
#include "ins.h"
#include "message_center.h"

static Publisher_t *g_imu_publisher;
#endif

void RobotInit()
{  
    __disable_irq();
    
    BSPInit();

#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
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

void RobotOLEDTask(void)
{
#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
    if (!robot_oled_initialized) {
        if (OLED_init_ex() != DEVICE_OK) {
            robot_oled_error_count++;
            return;
        }
        robot_oled_initialized = true;
    }

    OLED_operate_gram(PEN_CLEAR);

#if defined(ROBOT_ENABLE_CHASSIS_APP)
    const int left_target = (int) chassis_motors[0].target_speed;
    const int left_measured = (int) chassis_motors[0].measured_speed;
    const int right_target = (int) chassis_motors[1].target_speed;
    const int right_measured = (int) chassis_motors[1].measured_speed;
    const int left_output =
        (int) (chassis_motors[0].control_output * 1000.0f);
    const int right_output =
        (int) (chassis_motors[1].control_output * 1000.0f);
    const int vx_mmps = (int) (chassis_manual_vx_mps * 1000.0f);
    const int wz_mradps = (int) (chassis_manual_wz_radps * 1000.0f);

    const bool motor_enabled =
        chassis_motors[0].enabled || chassis_motors[1].enabled;
    OLED_printf(0U, 0U, "M:%s CMD:%s",
        motor_enabled ? "ON" : "OFF",
        chassis_manual_enabled ? "ON" : "OFF");
    OLED_printf(1U, 0U, "LT:%6d LM:%6d", left_target, left_measured);
    OLED_printf(2U, 0U, "RT:%6d RM:%6d", right_target, right_measured);
    OLED_printf(3U, 0U, "LO:%4d RO:%4d", left_output, right_output);
    OLED_printf(4U, 0U, "V:%5d W:%5d", vx_mmps, wz_mradps);
#else
    OLED_printf(0U, 0U, "ROBOT READY");
#endif

    OLED_refresh_gram();
    robot_oled_refresh_count++;
#endif
}
