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

/* ====================== I2C1 JY901S pinmux override ====================== */
/*
 * Switch I2C from I2C0 (PA28/PA31) to I2C1 (PB2=SCL, PB3=SDA).
 * PB2 = PINCM15, PF=0x4 -> I2C1_SCL
 * PB3 = PINCM16, PF=0x4 -> I2C1_SDA
 */
static void RobotReconfigI2C1(void)
{
    /* 1. Power on and init I2C1 (SysConfig only handles I2C0). */
    DL_I2C_reset(I2C1);
    DL_I2C_enablePower(I2C1);
    delay_cycles(POWER_STARTUP_DELAY);

    static const DL_I2C_ClockConfig clk_cfg = {
        .clockSel    = DL_I2C_CLOCK_BUSCLK,
        .divideRatio = DL_I2C_CLOCK_DIVIDE_1,
    };
    DL_I2C_setClockConfig(I2C1, (DL_I2C_ClockConfig *)&clk_cfg);
    DL_I2C_setAnalogGlitchFilterPulseWidth(
        I2C1, DL_I2C_ANALOG_GLITCH_FILTER_WIDTH_50NS);
    DL_I2C_enableAnalogGlitchFilter(I2C1);

    DL_I2C_resetControllerTransfer(I2C1);
    DL_I2C_setTimerPeriod(I2C1, 39);
    DL_I2C_setControllerTXFIFOThreshold(
        I2C1, DL_I2C_TX_FIFO_LEVEL_EMPTY);
    DL_I2C_setControllerRXFIFOThreshold(
        I2C1, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
    DL_I2C_enableControllerClockStretching(I2C1);
    DL_I2C_enableController(I2C1);

    /* 2. Route PB2 (PINCM15) -> I2C1_SCL (PF=0x4). */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM15] =
        (IOMUX->SECCFG.PINCM[IOMUX_PINCM15] & ~0xFU) |
        IOMUX_PINCM15_PF_I2C1_SCL;
    DL_GPIO_enableHiZ(IOMUX_PINCM15);

    /* 3. Route PB3 (PINCM16) -> I2C1_SDA (PF=0x4). */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM16] =
        (IOMUX->SECCFG.PINCM[IOMUX_PINCM16] & ~0xFU) |
        IOMUX_PINCM16_PF_I2C1_SDA;
    DL_GPIO_enableHiZ(IOMUX_PINCM16);
}

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

    /* Reconfigure I2C to I2C1 on PB2(SCL)/PB3(SDA) for JY901S. */
    RobotReconfigI2C1();

    {
        IIC_Init_Config_s iic_config = {
            .handle = &hi2c1,
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