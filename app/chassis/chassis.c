#include "chassis.h"
#include "robot_def.h"

#include "bsp_dwt.h"
#include "motor.h"

#if defined(ROBOT_ENABLE_INS_APP)
#include "ins.h"
#include "message_center.h"
#endif

#include "ti_msp_dl_config.h"

/* ====================== Chassis Configuration ====================== */
#define CHASSIS_MOTOR_COUNT 2U

#define CHASSIS_MOTOR_PWM_PERIOD_SEC (0.00005f)

/* ====================== Motor Pin Definitions ====================== */
#define MOTORA_PWM_HTIM  (&htim1)
#define MOTORA_PHASE_PORT MOTOR_GPIO_AIN1_PORT
#define MOTORA_PHASE_PIN  MOTOR_GPIO_AIN1_PIN

#define MOTORB_PWM_HTIM  (&htim2)
#define MOTORB_PHASE_PORT MOTOR_GPIO_BIN1_PORT
#define MOTORB_PHASE_PIN  MOTOR_GPIO_BIN1_PIN

/* ====================== Data Structures ====================== */
Motor_Device_t             chassis_motors[CHASSIS_MOTOR_COUNT];
static Chassis_Ctrl_Cmd_s  g_cmd;
static bool                g_chassis_initialized;
static bool                g_control_time_initialized;
static uint32_t            g_control_timestamp_us;

volatile bool chassis_manual_enabled;
volatile float chassis_manual_vx_mps;
volatile float chassis_manual_wz_radps;

#if defined(ROBOT_ENABLE_INS_APP)
static Publisher_t         *g_encoder_publisher;
static Subscriber_t        *g_command_subscriber;
#endif

static void ChassisReceiveCommand(void);
static void ChassisUpdateMode(void);
static void ChassisPublishFeedback(void);

static bool ChassisInitMotor(Motor_Device_t *motor,
    TIM_HandleTypeDef *pwm_handle, GPIO_TypeDef *phase_port,
    uint32_t phase_pin, Encoder_Device_t *encoder, bool reverse)
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
                .reverse = reverse,
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
        .encoder_reverse = reverse,
    };
    return Motor_Init(motor, &config);
}

/* ====================== Public API ====================== */

void ChassisInit(void)
{
    if (g_chassis_initialized) {
        return;
    }

    if (!ChassisInitMotor(&chassis_motors[0], MOTORA_PWM_HTIM,
        MOTORA_PHASE_PORT, MOTORA_PHASE_PIN, &hencoder_left,
        (bool) CHASSIS_LEFT_MOTOR_REVERSE)) {
        return;
    }

    if (!ChassisInitMotor(&chassis_motors[1], MOTORB_PWM_HTIM,
        MOTORB_PHASE_PORT, MOTORB_PHASE_PIN, &hencoder_right,
        (bool) CHASSIS_RIGHT_MOTOR_REVERSE)) {
        return;
    }

    g_cmd.vx = 0.0f;
    g_cmd.vy = 0.0f;
    g_cmd.wz = 0.0f;
    g_cmd.chassis_mode = CHASSIS_ZERO_FORCE;

#if defined(ROBOT_ENABLE_INS_APP)
    g_encoder_publisher = PubRegister(
        INS_ENCODER_TOPIC, sizeof(Encoder_Pub_Data_t));
    g_command_subscriber = SubRegister(
        INS_CMD_TOPIC, sizeof(INS_ChassisCommand_t));
#endif

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
            Motor_Stop(&chassis_motors[i]);
        }
    } else {
        for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
            if (!chassis_motors[i].enabled) {
                Motor_Enable(&chassis_motors[i]);
            }
        }
        ChassisUpdateMode();
        for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
            (void) Motor_Update(&chassis_motors[i], dt_seconds);
        }
    }

    ChassisPublishFeedback();
}

void ChassisSetManualCommand(float vx_mps, float wz_radps)
{
    chassis_manual_enabled = false;
    chassis_manual_vx_mps = vx_mps;
    chassis_manual_wz_radps = wz_radps;
    chassis_manual_enabled = true;
}

void ChassisDisableManualCommand(void)
{
    chassis_manual_enabled = false;
    chassis_manual_vx_mps = 0.0f;
    chassis_manual_wz_radps = 0.0f;
}

static void ChassisReceiveCommand(void)
{
#if defined(ROBOT_ENABLE_INS_APP)
    INS_ChassisCommand_t command;
    if ((g_command_subscriber != NULL) &&
        (SubGetMessage(g_command_subscriber, &command) != 0U) &&
        command.motion_enabled) {
        g_cmd.vx = command.vx;
        g_cmd.vy = command.vy;
        g_cmd.wz = command.wz;
        g_cmd.chassis_mode = CHASSIS_ROTATE;
    } else {
        g_cmd.vx = 0.0f;
        g_cmd.vy = 0.0f;
        g_cmd.wz = 0.0f;
        g_cmd.chassis_mode = CHASSIS_ZERO_FORCE;
    }
#else
    g_cmd.vx = chassis_manual_vx_mps;
    g_cmd.vy = 0.0f;
    g_cmd.wz = chassis_manual_wz_radps;
    g_cmd.chassis_mode = chassis_manual_enabled
        ? CHASSIS_ROTATE
        : CHASSIS_ZERO_FORCE;
#endif
}

static void ChassisUpdateMode(void)
{
    if (g_cmd.chassis_mode == CHASSIS_ZERO_FORCE) {
        return;
    }

    float half_track = CHASSIS_TRACK_WIDTH_M * 0.5f;
    float v_left     = g_cmd.vx - g_cmd.wz * half_track;
    float v_right    = g_cmd.vx + g_cmd.wz * half_track;

    const float two_pi_r = 2.0f * 3.14159265f * CHASSIS_WHEEL_RADIUS_M;
    const float conv = (CHASSIS_ENCODER_PPR *
        CHASSIS_ENCODER_QUADRATURE * CHASSIS_MOTOR_GEAR_RATIO) /
        two_pi_r;

    Motor_SetTargetSpeed(&chassis_motors[0], v_left * conv);
    Motor_SetTargetSpeed(&chassis_motors[1], v_right * conv);
}

static void ChassisPublishFeedback(void)
{
#if defined(ROBOT_ENABLE_INS_APP)
    if (g_encoder_publisher != NULL) {
        Encoder_Pub_Data_t encoder_data = {
            .left_total = Encoder_Get_Total(chassis_motors[0].encoder),
            .right_total = Encoder_Get_Total(chassis_motors[1].encoder),
        };
        (void) PubPushMessage(g_encoder_publisher, &encoder_data);
    }
#endif
}
