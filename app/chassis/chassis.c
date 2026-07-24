#include "chassis.h"
#include "robot_def.h"

#include "bsp_encoder.h"
#include "bsp_gpio.h"
#include "bsp_pwm.h"
#include "motor.h"
#include "pid.h"
#include "bsp_log.h"
#include "message_center.h"
#include "ins.h"

#include "ti_msp_dl_config.h"

/* ====================== Chassis Configuration ====================== */
#define CHASSIS_MOTOR_COUNT 2U

#define CHASSIS_DT_SEC (0.005f)

#ifndef CHASSIS_WHEEL_RADIUS_M
#define CHASSIS_WHEEL_RADIUS_M (0.076f)
#endif
#ifndef CHASSIS_TRACK_WIDTH_M
#define CHASSIS_TRACK_WIDTH_M (0.40f)
#endif

#define CHASSIS_SPEED_KP  (5.0f)
#define CHASSIS_SPEED_KI  (0.2f)
#define CHASSIS_SPEED_KD  (0.0f)
#define CHASSIS_SPEED_MAX_OUT   (1000.0f)
#define CHASSIS_SPEED_MAX_IOUT  (500.0f)

/* ====================== Motor Pin Definitions ====================== */
#define MOTORA_PWM_HTIM  (&htim1)
#define MOTORA_PWM_CHAN  TIM_CHANNEL_1
#define MOTORA_DIR1_PORT GPIOA
#define MOTORA_DIR1_PIN  GPIO_PIN_17
#define MOTORA_DIR2_PORT GPIOA
#define MOTORA_DIR2_PIN  GPIO_PIN_16

#define MOTORB_PWM_HTIM  (&htim2)
#define MOTORB_PWM_CHAN  TIM_CHANNEL_2
#define MOTORB_DIR1_PORT GPIOB
#define MOTORB_DIR1_PIN  GPIO_PIN_4
#define MOTORB_DIR2_PORT GPIOB
#define MOTORB_DIR2_PIN  GPIO_PIN_1

/* ====================== Data Structures ====================== */
typedef struct {
    Motor_Device_t   motor;
    PID_Device_t     speed_pid;
    Encoder_Device_t encoder;
    int16_t          target_speed;
    int16_t          actual_speed;
} ChassisMotor_t;

static ChassisMotor_t      g_motors[CHASSIS_MOTOR_COUNT];
static Chassis_Ctrl_Cmd_s  g_cmd;
static bool                g_chassis_initialized;

/* Message center handles */
static Publisher_t   *g_enc_pub;
static Subscriber_t  *g_cmd_sub;

static void ChassisReceiveCommand(void);
static void ChassisUpdateMode(void);
static void ChassisPublishFeedback(void);

static void ChassisSetMotorOutput(uint8_t idx, int32_t output)
{
    if (idx >= CHASSIS_MOTOR_COUNT) {
        return;
    }
    Motor_SetSpeed(&g_motors[idx].motor, output);
}

/* ====================== TIMA0 PWM setup ====================== */
static void ChassisReconfigTIMA0(void)
{
    DL_TimerA_stopCounter(TIMER_0_INST);

    static const DL_TimerA_ClockConfig clock_cfg = {
        .clockSel    = DL_TIMER_CLOCK_BUSCLK,
        .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    };
    DL_TimerA_setClockConfig(TIMER_0_INST,
        (DL_TimerA_ClockConfig *)&clock_cfg);

    DL_TimerA_setLoadValue(TIMER_0_INST, 3999U);

    static const DL_TimerA_TimerConfig pwm_cfg = {
        .timerMode    = DL_TIMER_TIMER_MODE_PERIODIC,
        .period       = 3999U,
        .startTimer   = DL_TIMER_STOP,
        .genIntermInt = DL_TIMER_INTERM_INT_DISABLED,
        .counterVal   = 0U,
    };
    DL_TimerA_initTimerMode(TIMER_0_INST,
        (DL_TimerA_TimerConfig *)&pwm_cfg);

    /* Route TIMA0 CCP0 -> PA0 (PINCM1, PF=4). */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM1] =
        (IOMUX->SECCFG.PINCM[IOMUX_PINCM1] & ~0xFU) |
        IOMUX_PINCM1_PF_TIMA0_CCP0;
    DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_0);

    /* Route TIMA0 CCP1 -> PA1 (PINCM2, PF=4). */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM2] =
        (IOMUX->SECCFG.PINCM[IOMUX_PINCM2] & ~0xFU) |
        IOMUX_PINCM2_PF_TIMA0_CCP1;
    DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_1);
}

/* ====================== Public API ====================== */

void ChassisInit(void)
{
    if (g_chassis_initialized) {
        return;
    }

    ChassisReconfigTIMA0();

    /* Configure motor direction pins as GPIO outputs
     * (overrides SysConfig PWM_3 on PA16/PA17).
     *   AIN1=PA17 (PINCM39), AIN2=PA16 (PINCM38),
     *   BIN1=PB4  (PINCM17), BIN2=PB1  (PINCM13) */
    DL_GPIO_initDigitalOutput(IOMUX_PINCM39);
    DL_GPIO_clearPins(GPIOA, DL_GPIO_PIN_17);
    DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_17);

    DL_GPIO_initDigitalOutput(IOMUX_PINCM38);
    DL_GPIO_clearPins(GPIOA, DL_GPIO_PIN_16);
    DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_16);

    DL_GPIO_initDigitalOutput(IOMUX_PINCM17);
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_4);
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_4);

    DL_GPIO_initDigitalOutput(IOMUX_PINCM13);
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_1);
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_1);

    /* --- Motor A (Left) --- */
    {
        GPIO_Init_Config_s cfg0 = {
            .GPIOx = MOTORA_DIR1_PORT, .GPIO_Pin = MOTORA_DIR1_PIN,
            .pin_state = GPIO_PIN_RESET, .exti_mode = GPIO_EXTI_MODE_NONE,
            .gpio_model_callback = NULL, .id = NULL,
        };
        GPIOInstance *dir1 = GPIORegister(&cfg0);

        GPIO_Init_Config_s cfg1 = {
            .GPIOx = MOTORA_DIR2_PORT, .GPIO_Pin = MOTORA_DIR2_PIN,
            .pin_state = GPIO_PIN_RESET, .exti_mode = GPIO_EXTI_MODE_NONE,
            .gpio_model_callback = NULL, .id = NULL,
        };
        GPIOInstance *dir2 = GPIORegister(&cfg1);

        PWM_Init_Config_s pwm_cfg = {
            .htim = MOTORA_PWM_HTIM, .channel = MOTORA_PWM_CHAN,
            .period = 0.00005f, .dutyratio = 0.0f,
            .callback = NULL, .id = NULL,
        };
        PWMInstance *pwm = PWMRegister(&pwm_cfg);

        Motor_Device_t *m = &g_motors[0].motor;
        m->pwm_pin   = pwm;
        m->dir_in1   = dir1;
        m->dir_in2   = dir2;
        m->reverse   = false;
        m->stop_mode = MOTOR_STOP_COAST;
        Motor_Init(m);

        PID_Init(&g_motors[0].speed_pid, PID_POSITION,
                 CHASSIS_SPEED_KP, CHASSIS_SPEED_KI, CHASSIS_SPEED_KD,
                 CHASSIS_SPEED_MAX_OUT, CHASSIS_SPEED_MAX_IOUT);

        Encoder_Device_t *enc = &g_motors[0].encoder;
        enc->port_a = GPIOA;
        enc->pin_a  = DL_GPIO_PIN_12;
        enc->port_b = GPIOA;
        enc->pin_b  = DL_GPIO_PIN_13;
        Encoder_Init(enc);
    }

    /* --- Motor B (Right) --- */
    {
        GPIO_Init_Config_s cfg0 = {
            .GPIOx = MOTORB_DIR1_PORT, .GPIO_Pin = MOTORB_DIR1_PIN,
            .pin_state = GPIO_PIN_RESET, .exti_mode = GPIO_EXTI_MODE_NONE,
            .gpio_model_callback = NULL, .id = NULL,
        };
        GPIOInstance *dir1 = GPIORegister(&cfg0);

        GPIO_Init_Config_s cfg1 = {
            .GPIOx = MOTORB_DIR2_PORT, .GPIO_Pin = MOTORB_DIR2_PIN,
            .pin_state = GPIO_PIN_RESET, .exti_mode = GPIO_EXTI_MODE_NONE,
            .gpio_model_callback = NULL, .id = NULL,
        };
        GPIOInstance *dir2 = GPIORegister(&cfg1);

        PWM_Init_Config_s pwm_cfg = {
            .htim = MOTORB_PWM_HTIM, .channel = MOTORB_PWM_CHAN,
            .period = 0.00005f, .dutyratio = 0.0f,
            .callback = NULL, .id = NULL,
        };
        PWMInstance *pwm = PWMRegister(&pwm_cfg);

        Motor_Device_t *m = &g_motors[1].motor;
        m->pwm_pin   = pwm;
        m->dir_in1   = dir1;
        m->dir_in2   = dir2;
        m->reverse   = false;
        m->stop_mode = MOTOR_STOP_COAST;
        Motor_Init(m);

        PID_Init(&g_motors[1].speed_pid, PID_POSITION,
                 CHASSIS_SPEED_KP, CHASSIS_SPEED_KI, CHASSIS_SPEED_KD,
                 CHASSIS_SPEED_MAX_OUT, CHASSIS_SPEED_MAX_IOUT);

        Encoder_Device_t *enc = &g_motors[1].encoder;
        enc->port_a = GPIOB;
        enc->pin_a  = DL_GPIO_PIN_22;
        enc->port_b = GPIOB;
        enc->pin_b  = DL_GPIO_PIN_23;
        Encoder_Init(enc);
    }

    g_cmd.vx = 0.0f;
    g_cmd.vy = 0.0f;
    g_cmd.wz = 0.0f;
    g_cmd.chassis_mode = CHASSIS_ZERO_FORCE;

    /* Register encoder data publisher for INS */
    g_enc_pub = PubRegister(INS_ENCODER_TOPIC, sizeof(Encoder_Pub_Data_t));
    if (g_enc_pub == NULL) {
        LOGERROR("[Chassis] Failed to register encoder_data publisher");
    }

    /* Subscribe to chassis commands from INS */
    g_cmd_sub = SubRegister(INS_CMD_TOPIC, sizeof(Chassis_Cmd_Pub_t));
    if (g_cmd_sub == NULL) {
        LOGERROR("[Chassis] Failed to subscribe to chassis_cmd");
    }

    g_chassis_initialized = true;
}

void ChassisTask(void)
{
    if (!g_chassis_initialized) {
        return;
    }

    ChassisReceiveCommand();

    Encoder_Update(&g_motors[0].encoder);
    Encoder_Update(&g_motors[1].encoder);

    g_motors[0].actual_speed = Encoder_Get_Speed(&g_motors[0].encoder);
    g_motors[1].actual_speed = Encoder_Get_Speed(&g_motors[1].encoder);

    ChassisUpdateMode();

    for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
        if (g_cmd.chassis_mode == CHASSIS_ZERO_FORCE) {
            ChassisSetMotorOutput(i, 0);
            PID_Clear(&g_motors[i].speed_pid);
        } else {
            float out = PID_Calc(&g_motors[i].speed_pid,
                                 (float)g_motors[i].actual_speed,
                                 (float)g_motors[i].target_speed);
            ChassisSetMotorOutput(i, (int32_t)out);
        }
    }

    ChassisPublishFeedback();
}

static void ChassisReceiveCommand(void)
{
    /* Try to read chassis command from INS via message center */
    Chassis_Cmd_Pub_t ins_cmd;
    if ((g_cmd_sub != NULL) && SubGetMessage(g_cmd_sub, &ins_cmd)) {
        g_cmd.vx = ins_cmd.vx;
        g_cmd.vy = ins_cmd.vy;
        g_cmd.wz = ins_cmd.wz;
        g_cmd.offset_angle = ins_cmd.offset_angle;
        g_cmd.chassis_mode  = (ins_cmd.chassis_mode != 0U)
                              ? CHASSIS_ROTATE : CHASSIS_ZERO_FORCE;
        g_cmd.chassis_speed_buff = ins_cmd.speed_buff;
    } else {
        /* Fallback: hard-coded test command when no INS command */
        g_cmd.vx = 0.3f;
        g_cmd.vy = 0.0f;
        g_cmd.wz = 0.0f;
        g_cmd.chassis_mode = CHASSIS_ROTATE;
    }
}

static void ChassisUpdateMode(void)
{
    if (g_cmd.chassis_mode == CHASSIS_ZERO_FORCE) {
        for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
            g_motors[i].target_speed = 0;
        }
        return;
    }

    float half_track = CHASSIS_TRACK_WIDTH_M * 0.5f;
    float v_left     = g_cmd.vx - g_cmd.wz * half_track;
    float v_right    = g_cmd.vx + g_cmd.wz * half_track;

    const float ppr      = 11.0f;
    const float gear     = 19.0f;
    const float two_pi_r = 2.0f * 3.14159265f * CHASSIS_WHEEL_RADIUS_M;
    const float conv     = (ppr * 4.0f * gear * CHASSIS_DT_SEC) / two_pi_r;

    g_motors[0].target_speed = (int16_t)(v_left  * conv);
    g_motors[1].target_speed = (int16_t)(v_right * conv);
}

static void ChassisPublishFeedback(void)
{
    /* Publish encoder totals to message center for INS dead reckoning */
    if (g_enc_pub != NULL) {
        Encoder_Pub_Data_t enc_data;
        enc_data.left_total  = Encoder_Get_Total(&g_motors[0].encoder);
        enc_data.right_total = Encoder_Get_Total(&g_motors[1].encoder);
        PubPushMessage(g_enc_pub, &enc_data);
    }
}
