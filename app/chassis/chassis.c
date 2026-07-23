#include "chassis.h"
#include "robot_def.h"

#include "bsp_encoder.h"
#include "bsp_gpio.h"
#include "bsp_pwm.h"
#include "motor.h"
#include "pid.h"

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
#define MOTORA_DIR1_PORT GPIOA
#define MOTORA_DIR1_PIN  GPIO_PIN_17
#define MOTORA_DIR2_PORT GPIOA
#define MOTORA_DIR2_PIN  GPIO_PIN_16

#define MOTORB_PWM_HTIM  (&htim2)
#define MOTORB_DIR1_PORT GPIOB
#define MOTORB_DIR1_PIN  GPIO_PIN_4
#define MOTORB_DIR2_PORT GPIOB
#define MOTORB_DIR2_PIN  GPIO_PIN_1

/* ====================== Data Structures ====================== */
typedef struct {
    Motor_Device_t   motor;
    PID_Device_t     speed_pid;
    Encoder_Device_t *encoder;
    int16_t          target_speed;
    int16_t          actual_speed;
} ChassisMotor_t;

static ChassisMotor_t      g_motors[CHASSIS_MOTOR_COUNT];
static Chassis_Ctrl_Cmd_s  g_cmd;
static bool                g_chassis_initialized;

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

/* ====================== Public API ====================== */

void ChassisInit(void)
{
    if (g_chassis_initialized) {
        return;
    }

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
            .htim = MOTORA_PWM_HTIM, .channel = MOTORA_PWM_HTIM->Channel,
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

        g_motors[0].encoder = &hencoder_left;
        Encoder_Start(g_motors[0].encoder);
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
            .htim = MOTORB_PWM_HTIM, .channel = MOTORB_PWM_HTIM->Channel,
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

        g_motors[1].encoder = &hencoder_right;
        Encoder_Start(g_motors[1].encoder);
    }

    g_cmd.vx = 0.0f;
    g_cmd.vy = 0.0f;
    g_cmd.wz = 0.0f;
    g_cmd.chassis_mode = CHASSIS_ZERO_FORCE;

    g_chassis_initialized = true;
}

void ChassisTask(void)
{
    if (!g_chassis_initialized) {
        return;
    }

    ChassisReceiveCommand();

    Encoder_Update(g_motors[0].encoder);
    Encoder_Update(g_motors[1].encoder);

    g_motors[0].actual_speed = Encoder_Get_Speed(g_motors[0].encoder);
    g_motors[1].actual_speed = Encoder_Get_Speed(g_motors[1].encoder);

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
    g_cmd.vx = 0.3f;
    g_cmd.vy = 0.0f;
    g_cmd.wz = 0.0f;
    g_cmd.chassis_mode = CHASSIS_ROTATE;
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
}
