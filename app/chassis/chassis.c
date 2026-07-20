/**
 * @file chassis.c
 * @brief 底盘巡线应用 -- 双电机差速 + 8路灰度 + PID巡线
 *
 * 硬件接线:
 *   左电机(A): PWM=PA0(TIMA0 CC0), AIN1=PA17, AIN2=PA16
 *   右电机(B): PWM=PA1(TIMA0 CC1), BIN1=PB4,  BIN2=PB1
 *   编码器L:   TIMG12 (CAPTURE_0, PB13)
 *   编码器R:   TIMG8  (CAPTURE_1, PA26)
 *
 *   灰度从左到右: L4(PB18) L3(PA18) L2(PA24) L1(PA15) | R1(PA22) R2(PB19) R3(PA29) R4(PB20)
 */
#include "chassis.h"
#include "robot_def.h"
#include "gray_sensor.h"
#include "motor.h"
#include "pid.h"
#include "bsp_gpio.h"
#include "bsp_pwm.h"
#include "bsp_encoder.h"
#include <string.h>

/* 硬件句柄 */
extern TIM_HandleTypeDef htim1;  /* TIMA0, 左右电机共用定时器 */
extern TIM_HandleTypeDef htim5;  /* TIMG12, 编码器L */
extern TIM_HandleTypeDef htim6;  /* TIMG8,  编码器R */

/* 编码器实例 */
static Encoder_Device_t ENCODER_L = { .htim = &htim5 };
static Encoder_Device_t ENCODER_R = { .htim = &htim6 };

/* 电机实例 */
static Motor_Device_t motor_left;
static Motor_Device_t motor_right;

/* 灰度传感器 -- 物理排列（左到右）：L4, L3, L2, L1, R1, R2, R3, R4 */
static Gray_Sensor_t gray_sensor;

static const struct { GPIO_TypeDef *port; uint32_t pin; } gray_pin_cfg[8] = {
    {GPIOB, GPIO_PIN_18},  /* [0] L4: PB18 */
    {GPIOA, GPIO_PIN_18},  /* [1] L3: PA18 */
    {GPIOA, GPIO_PIN_24},  /* [2] L2: PA24 */
    {GPIOA, GPIO_PIN_15},  /* [3] L1: PA15 */
    {GPIOA, GPIO_PIN_22},  /* [4] R1: PA22 */
    {GPIOB, GPIO_PIN_19},  /* [5] R2: PB19 */
    {GPIOA, GPIO_PIN_29},  /* [6] R3: PA29 */
    {GPIOB, GPIO_PIN_20},  /* [7] R4: PB20 */
};

/* PID 巡线控制器 */
static PID_Device_t pid_line;

#define LINE_KP       2.0f    /* 比例系数 */
#define LINE_KI       0.1f    /* 积分系数 */
#define LINE_KD       0.5f    /* 微分系数 */
#define LINE_MAX_IOUT 500.0f  /* 积分限幅，防饱和 */
#define LINE_MAX_OUT  1500.0f /* 输出限幅，防烧电机 */
#define BASE_SPEED    350     /* 基础 PWM 占空比，范围 0 ~ 1000 */

/* 共享控制 / 反馈数据 */
static Chassis_Ctrl_Cmd_s    chassis_cmd_recv;
static Chassis_Upload_Data_s chassis_feedback_data;

/* 内部流水线声明 */
static void ChassisReceiveCommand(void);
static void ChassisUpdateMode(void);
static void ChassisKinematicsSolve(void);
static void ChassisApplyOutput(void);
static void ChassisUpdateFeedback(void);
static void ChassisPublishFeedback(void);

/* ================================================================
 * 公开 API
 * ================================================================ */

void ChassisInit(void)
{
    /*
     * 第一阶段：注册 8 路灰度 GPIO（输入模式，外部上拉）
     * 灰度传感器红外对管为开漏输出，高电平对应白线，低电平对应黑线
     */
    memset(&gray_sensor, 0, sizeof(gray_sensor));
    for (int i = 0; i < 8; i++) {
        GPIO_Init_Config_s cfg = {
            .GPIOx     = gray_pin_cfg[i].port,
            .GPIO_Pin  = gray_pin_cfg[i].pin,
            .pin_state = GPIO_PIN_RESET,
            .exti_mode = GPIO_EXTI_MODE_NONE,
            .gpio_model_callback = NULL,
            .id        = NULL,
        };
        gray_sensor.sensors[i].sensor = GPIORegister(&cfg);
        gray_sensor.sensors[i].value  = 0;
    }
    Gray_Sensor_Init(&gray_sensor);

    /*
     * 第二阶段：注册左电机（A）
     * PWM: PA0 = TIMA0 CC0, 方向: AIN1=PA17, AIN2=PA16
     */
    {
        PWM_Init_Config_s pwm_cfg = {
            .htim      = &htim1,
            .channel   = TIM_CHANNEL_1,   /* TIMA0 CC0 = PA0 */
            .period    = 0.00005f,        /* 20 kHz */
            .dutyratio = 0.0f,
            .callback  = NULL,
            .id        = NULL,
        };
        PWMInstance *pwm_l = PWMRegister(&pwm_cfg);

        GPIO_Init_Config_s g = {
            .GPIOx     = GPIOA,
            .GPIO_Pin  = GPIO_PIN_17,     /* AIN1 */
            .pin_state = GPIO_PIN_SET,    /* 初始化拉高，刹车态 */
            .exti_mode = GPIO_EXTI_MODE_NONE,
            .gpio_model_callback = NULL,
            .id        = NULL,
        };
        GPIOInstance *ain1 = GPIORegister(&g);

        g.GPIO_Pin = GPIO_PIN_16;         /* AIN2 */
        GPIOInstance *ain2 = GPIORegister(&g);

        motor_left.pwm_pin = pwm_l;
        motor_left.dir_in1 = ain1;
        motor_left.dir_in2 = ain2;
        motor_left.reverse = false;
        Motor_Init(&motor_left);
    }

    /*
     * 第三阶段：注册右电机（B）
     * PWM: PA1 = TIMA0 CC1, 方向: BIN1=PB4, BIN2=PB1
     */
    {
        PWM_Init_Config_s pwm_cfg = {
            .htim      = &htim1,
            .channel   = TIM_CHANNEL_2,   /* TIMA0 CC1 = PA1 */
            .period    = 0.00005f,
            .dutyratio = 0.0f,
            .callback  = NULL,
            .id        = NULL,
        };
        PWMInstance *pwm_r = PWMRegister(&pwm_cfg);

        GPIO_Init_Config_s g = {
            .GPIOx     = GPIOB,
            .GPIO_Pin  = GPIO_PIN_4,      /* BIN1 */
            .pin_state = GPIO_PIN_SET,
            .exti_mode = GPIO_EXTI_MODE_NONE,
            .gpio_model_callback = NULL,
            .id        = NULL,
        };
        GPIOInstance *bin1 = GPIORegister(&g);

        g.GPIOx    = GPIOB;
        g.GPIO_Pin = GPIO_PIN_1;          /* BIN2 */
        GPIOInstance *bin2 = GPIORegister(&g);

        motor_right.pwm_pin = pwm_r;
        motor_right.dir_in1 = bin1;
        motor_right.dir_in2 = bin2;
        motor_right.reverse = false;
        Motor_Init(&motor_right);
    }

    /* 第四阶段：启动编码器 */
    Encoder_Start(&ENCODER_L);
    Encoder_Start(&ENCODER_R);

    /* 第五阶段：初始化巡线 PID（位置式） */
    PID_Init(&pid_line, PID_POSITION,
        LINE_KP, LINE_KI, LINE_KD, LINE_MAX_OUT, LINE_MAX_IOUT);
}

/* ================================================================
 * 内部流水线
 * ================================================================ */

static void ChassisReceiveCommand(void)
{
    /* 预留：从 robot_cmd 或遥控器接收底盘控制命令 */
    (void)&chassis_cmd_recv;
}

static void ChassisUpdateMode(void)
{
    /* 预留：根据命令更新底盘工作模式 */
    (void)&chassis_cmd_recv;
}

static void ChassisKinematicsSolve(void)
{
    /* 巡线模式：运动学解算已集成在 ChassisTask 的差速输出中 */
}

static void ChassisApplyOutput(void)
{
    /* 电机输出已在下层 Motor_SetSpeed 中直接下发 */
}

static void ChassisUpdateFeedback(void)
{
    /* 预留：采集底盘状态，编码器里程、速度等 */
    (void)&chassis_feedback_data;
}

static void ChassisPublishFeedback(void)
{
    /* 预留：发布底盘反馈给 robot_cmd 或 UI */
    (void)&chassis_feedback_data;
}

/* ================================================================
 * 主任务入口
 * ================================================================ */

void ChassisTask(void)
{
    /* 流水线1：接收命令并更新模式 */
    ChassisReceiveCommand();
    ChassisUpdateMode();

    /*
     * 流水线2：灰度巡线
     * 读取 8 路灰度偏移量，PID 计算转向修正，差速控制左右电机
     */
    float offset = Gray_Sensor_Get_Offset(&gray_sensor);

    /* 判断是否看到线，任意一路为高电平即视为有线 */
    bool has_line = false;
    for (int i = 0; i < 8; i++) {
        if (gray_sensor.sensors[i].value == 1) {
            has_line = true;
            break;
        }
    }

    if (has_line) {
        /* 目标偏移 0.0 表示线居中，PID 输出转向修正量 */
        float correction = PID_Calc(&pid_line, offset, 0.0f);

        /* 差速：左轮加速则右轮减速，反之亦然 */
        int32_t speed_l = (int32_t)((float)BASE_SPEED + correction);
        int32_t speed_r = (int32_t)((float)BASE_SPEED - correction);

        Motor_SetSpeed(&motor_left,  speed_l);
        Motor_SetSpeed(&motor_right, speed_r);
    } else {
        /* 丢线：停车并清空 PID 历史状态 */
        Motor_SetSpeed(&motor_left,  0);
        Motor_SetSpeed(&motor_right, 0);
        PID_Clear(&pid_line);
    }

    /* 流水线3：采集反馈并发布 */
    ChassisUpdateFeedback();
    ChassisPublishFeedback();
}