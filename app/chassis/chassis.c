#include "chassis.h"
#include "robot_def.h"

#include "bsp_dwt.h"
#include "motor.h"

#include "ti_msp_dl_config.h"

/* ====================== Chassis Configuration ====================== */
#define CHASSIS_MOTOR_COUNT 2U

#ifndef CHASSIS_WHEEL_RADIUS_M
#define CHASSIS_WHEEL_RADIUS_M (0.076f)
#endif
#ifndef CHASSIS_TRACK_WIDTH_M
#define CHASSIS_TRACK_WIDTH_M (0.40f)
#endif

#define CHASSIS_SPEED_KP  (0.000025f)
#define CHASSIS_SPEED_KI  (0.0002f)
#define CHASSIS_SPEED_KD  (0.0f)
#define CHASSIS_SPEED_MAX_OUT   (1.0f)
#define CHASSIS_SPEED_MAX_IOUT  (0.5f)
#define CHASSIS_MOTOR_PWM_PERIOD_SEC (0.00005f)

/* ====================== Motor Pin Definitions ====================== */
#define MOTORA_PWM_HTIM  (&htim1)
#define MOTORA_PHASE_PORT GPIOA
#define MOTORA_PHASE_PIN  GPIO_PIN_17

#define MOTORB_PWM_HTIM  (&htim2)
#define MOTORB_PHASE_PORT GPIOB
#define MOTORB_PHASE_PIN  GPIO_PIN_4

/* ====================== Data Structures ====================== */
static Motor_Device_t      g_motors[CHASSIS_MOTOR_COUNT];
static Chassis_Ctrl_Cmd_s  g_cmd;
static bool                g_chassis_initialized;
static bool                g_control_time_initialized;
static uint32_t            g_control_timestamp_us;

static void ChassisReceiveCommand(void);
static void ChassisUpdateMode(void);
static void ChassisPublishFeedback(void);

static bool ChassisInitMotor(Motor_Device_t *motor,
    TIM_HandleTypeDef *pwm_handle, GPIO_TypeDef *phase_port,
    uint32_t phase_pin, Encoder_Device_t *encoder)
{
    Motor_Init_Config_t config = {
        .driver = {
            .type = MOTOR_DRIVER_DRV8701E,
            .config.drv8701e = {
                .pwm_handle = pwm_handle,
                .pwm_channel = pwm_handle->Channel,
                .pwm_period = CHASSIS_MOTOR_PWM_PERIOD_SEC,
                .phase_port = phase_port,
                .phase_pin = phase_pin,
                .reverse = false,
            },
        },
        .encoder = encoder,
        .speed_pid = {
            .kp = CHASSIS_SPEED_KP,
            .ki = CHASSIS_SPEED_KI,
            .kd = CHASSIS_SPEED_KD,
            .output_limit = CHASSIS_SPEED_MAX_OUT,
            .integral_limit = CHASSIS_SPEED_MAX_IOUT,
            .deadband = 0.0f,
            .derivative_on_measurement = true,
        },
        .encoder_reverse = false,
    };
    return Motor_Init(motor, &config);
}

/* ====================== Public API ====================== */

void ChassisInit(void)
{
    if (g_chassis_initialized) {
        return;
    }

    if (!ChassisInitMotor(&g_motors[0], MOTORA_PWM_HTIM,
        MOTORA_PHASE_PORT, MOTORA_PHASE_PIN, &hencoder_left)) {
        return;
    }

    if (!ChassisInitMotor(&g_motors[1], MOTORB_PWM_HTIM,
        MOTORB_PHASE_PORT, MOTORB_PHASE_PIN, &hencoder_right)) {
        return;
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

    float dt_seconds = DWT_GetDeltaT(&g_control_timestamp_us);
    if (!g_control_time_initialized) {
        g_control_time_initialized = true;
        dt_seconds = 0.0f;
    }

    if (g_cmd.chassis_mode == CHASSIS_ZERO_FORCE) {
        for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
            Motor_Stop(&g_motors[i]);
        }
    } else {
        for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
            if (!g_motors[i].enabled) {
                Motor_Enable(&g_motors[i]);
            }
        }
        ChassisUpdateMode();
        for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
            (void) Motor_Update(&g_motors[i], dt_seconds);
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
        return;
    }

    float half_track = CHASSIS_TRACK_WIDTH_M * 0.5f;
    float v_left     = g_cmd.vx - g_cmd.wz * half_track;
    float v_right    = g_cmd.vx + g_cmd.wz * half_track;

    const float ppr      = 11.0f;
    const float gear     = 19.0f;
    const float two_pi_r = 2.0f * 3.14159265f * CHASSIS_WHEEL_RADIUS_M;
    const float conv     = (ppr * 4.0f * gear) / two_pi_r;

    Motor_SetTargetSpeed(&g_motors[0], v_left * conv);
    Motor_SetTargetSpeed(&g_motors[1], v_right * conv);
}

static void ChassisPublishFeedback(void)
{
}
