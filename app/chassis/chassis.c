#include "chassis.h"
#include "robot_def.h"

#include "bsp_dwt.h"
#include "motor.h"

#if defined(ROBOT_ENABLE_INS_APP)
#include "ins.h"
#include "message_center.h"
#endif

#include "ti_msp_dl_config.h"

#define CHASSIS_MOTOR_COUNT          (2U)
#define CHASSIS_MOTOR_PWM_PERIOD_SEC (0.00005f)

Motor_Device_t chassis_motors[CHASSIS_MOTOR_COUNT];

volatile bool chassis_manual_enabled;
volatile float chassis_manual_vx_mps;
volatile float chassis_manual_wz_radps;

static Chassis_Ctrl_Cmd_s g_command;
static uint32_t g_control_timestamp_us;
static bool g_initialized;
static bool g_control_time_initialized;

#if defined(ROBOT_ENABLE_INS_APP)
static Publisher_t *g_encoder_publisher;
static Subscriber_t *g_command_subscriber;
#endif

static bool ChassisInitMotor(Motor_Device_t *motor,
    TIM_HandleTypeDef *pwm_handle, GPIO_TypeDef *phase_port,
    uint32_t phase_pin, Encoder_Device_t *encoder, bool reverse);
static void ChassisClearCommand(void);
static void ChassisReceiveCommand(void);
static float ChassisControlDeltaTime(void);
static void ChassisApplyCommand(float dt_seconds);
static void ChassisSetWheelTargets(void);
static void ChassisPublishFeedback(void);

void ChassisInit(void)
{
    if (g_initialized) {
        return;
    }

    if (!ChassisInitMotor(&chassis_motors[0], &htim1,
        MOTOR_GPIO_AIN1_PORT, MOTOR_GPIO_AIN1_PIN, &hencoder_left,
        (bool) CHASSIS_LEFT_MOTOR_REVERSE)) {
        return;
    }

    if (!ChassisInitMotor(&chassis_motors[1], &htim2,
        MOTOR_GPIO_BIN1_PORT, MOTOR_GPIO_BIN1_PIN, &hencoder_right,
        (bool) CHASSIS_RIGHT_MOTOR_REVERSE)) {
        return;
    }

    ChassisClearCommand();

#if defined(ROBOT_ENABLE_INS_APP)
    g_encoder_publisher = PubRegister(
        INS_ENCODER_TOPIC, sizeof(Encoder_Pub_Data_t));
    g_command_subscriber = SubRegister(
        INS_CMD_TOPIC, sizeof(INS_ChassisCommand_t));
#endif

    g_initialized = true;
}

void ChassisTask(void)
{
    if (!g_initialized) {
        return;
    }

    ChassisReceiveCommand();
    ChassisApplyCommand(ChassisControlDeltaTime());
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

static void ChassisClearCommand(void)
{
    g_command = (Chassis_Ctrl_Cmd_s) {
        .chassis_mode = CHASSIS_ZERO_FORCE,
    };
}

static void ChassisReceiveCommand(void)
{
    ChassisClearCommand();

#if defined(ROBOT_ENABLE_INS_APP)
    INS_ChassisCommand_t command;
    if ((g_command_subscriber != NULL) &&
        (SubGetMessage(g_command_subscriber, &command) != 0U) &&
        command.motion_enabled) {
        g_command.vx = command.vx;
        g_command.vy = command.vy;
        g_command.wz = command.wz;
        g_command.chassis_mode = CHASSIS_ROTATE;
    }
#else
    if (chassis_manual_enabled) {
        g_command.vx = chassis_manual_vx_mps;
        g_command.wz = chassis_manual_wz_radps;
        g_command.chassis_mode = CHASSIS_ROTATE;
    }
#endif
}

static float ChassisControlDeltaTime(void)
{
    float dt_seconds = DWT_GetDeltaT(&g_control_timestamp_us);
    if (!g_control_time_initialized) {
        g_control_time_initialized = true;
        return 0.0f;
    }
    return dt_seconds;
}

static void ChassisApplyCommand(float dt_seconds)
{
    if (g_command.chassis_mode == CHASSIS_ZERO_FORCE) {
        for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
            Motor_Stop(&chassis_motors[i]);
        }
        return;
    }

    for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
        if (!chassis_motors[i].enabled) {
            Motor_Enable(&chassis_motors[i]);
        }
    }

    ChassisSetWheelTargets();
    for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
        (void) Motor_Update(&chassis_motors[i], dt_seconds);
    }
}

static void ChassisSetWheelTargets(void)
{
    const float half_track = CHASSIS_TRACK_WIDTH_M * 0.5f;
    const float left_mps = g_command.vx - g_command.wz * half_track;
    const float right_mps = g_command.vx + g_command.wz * half_track;
    const float counts_per_meter =
        (CHASSIS_ENCODER_PPR * CHASSIS_ENCODER_QUADRATURE *
            CHASSIS_MOTOR_GEAR_RATIO) /
        (2.0f * 3.14159265f * CHASSIS_WHEEL_RADIUS_M);

    Motor_SetTargetSpeed(&chassis_motors[0], left_mps * counts_per_meter);
    Motor_SetTargetSpeed(&chassis_motors[1], right_mps * counts_per_meter);
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
