#include "chassis.h"
#include "robot_def.h"

#include "bsp_dwt.h"
#include "gray_sensor.h"
#include "key.h"
#include "motor.h"

#if defined(ROBOT_ENABLE_INS_APP)
#include "ins.h"
#include "message_center.h"
#endif

#include "ti_msp_dl_config.h"

#define CHASSIS_MOTOR_COUNT          (2U)
#define CHASSIS_MOTOR_PWM_PERIOD_SEC (0.00005f)
#define CHASSIS_LINE_TRACE_GRAY_SETTLE_TIME_US 5U
#define CHASSIS_LINE_TRACE_DEFAULT_STOP_ACTIVE_COUNT 4U

Motor_Device_t chassis_motors[CHASSIS_MOTOR_COUNT];

volatile bool chassis_manual_enabled;
volatile float chassis_manual_vx_mps;
volatile float chassis_manual_wz_radps;
volatile Device_Status_e chassis_line_trace_init_status = DEVICE_ERROR;
volatile Chassis_LineTrace_State_e chassis_line_trace_state =
    CHASSIS_LINE_TRACE_IDLE;
volatile uint32_t chassis_line_trace_key1_press_count;
volatile uint32_t chassis_line_trace_key3_press_count;
volatile uint32_t chassis_line_trace_cross_count;
volatile uint32_t chassis_line_trace_raw;
volatile uint32_t chassis_line_trace_active_count;
volatile float chassis_line_trace_offset;
volatile float chassis_line_trace_base_vx_mps = 0.04f;
volatile float chassis_line_trace_turn_kp = 1.0f;
volatile uint32_t chassis_line_trace_stop_active_threshold =
    CHASSIS_LINE_TRACE_DEFAULT_STOP_ACTIVE_COUNT;
volatile uint32_t chassis_line_trace_key_active_level;
volatile uint32_t chassis_line_trace_gray_active_level = 1U;

static Chassis_Ctrl_Cmd_s g_command;
static uint32_t g_control_timestamp_us;
static bool g_initialized;
static bool g_control_time_initialized;
static KEY_Device_t line_trace_key1;
static KEY_Device_t line_trace_key3;
static GraySensorInstance *line_trace_sensor;
static bool line_trace_key1_was_pressed;
static bool line_trace_key3_was_pressed;
static bool line_trace_cross_latched;

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
static void ChassisLineTraceInit(void);
static void ChassisLineTraceTask(void);
static GPIO_PinState ChassisLineTraceKeyActiveState(void);
static GPIO_PinState ChassisLineTraceGrayActiveState(void);
static bool ChassisLineTraceKeyPressed(KEY_Device_t *key);
static void ChassisLineTraceStart(void);
static void ChassisLineTraceStop(Chassis_LineTrace_State_e next_state);

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
    ChassisLineTraceInit();

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

    ChassisLineTraceTask();
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

static GPIO_PinState ChassisLineTraceKeyActiveState(void)
{
    return (chassis_line_trace_key_active_level != 0U)
        ? GPIO_PIN_SET
        : GPIO_PIN_RESET;
}

static GPIO_PinState ChassisLineTraceGrayActiveState(void)
{
    return (chassis_line_trace_gray_active_level != 0U)
        ? GPIO_PIN_SET
        : GPIO_PIN_RESET;
}

static bool ChassisLineTraceKeyPressed(KEY_Device_t *key)
{
    key->active_state = ChassisLineTraceKeyActiveState();
    return KEY_IsPressed(key);
}

static void ChassisLineTraceStart(void)
{
    chassis_line_trace_state = CHASSIS_LINE_TRACE_RUNNING;
    chassis_line_trace_cross_count = 0U;
    line_trace_cross_latched = false;
}

static void ChassisLineTraceStop(Chassis_LineTrace_State_e next_state)
{
    ChassisDisableManualCommand();
    chassis_line_trace_state = next_state;
    line_trace_cross_latched = false;
}

static void ChassisLineTraceInit(void)
{
    bool key_ok =
        KEY_Init(&line_trace_key1, KEY_GPIO_KEY1_PORT, KEY_GPIO_KEY1_PIN,
            ChassisLineTraceKeyActiveState()) &&
        KEY_Init(&line_trace_key3, KEY_GPIO_KEY3_PORT, KEY_GPIO_KEY3_PIN,
            ChassisLineTraceKeyActiveState());

    GraySensor_Init_Config_s gray_config = {
        .ad0_port = GRAY_SENSOR_GPIO_PORT,
        .ad0_pin = GRAY_SENSOR_GPIO_AD0_PIN,
        .ad1_port = GRAY_SENSOR_GPIO_PORT,
        .ad1_pin = GRAY_SENSOR_GPIO_AD1_PIN,
        .ad2_port = GRAY_SENSOR_GPIO_PORT,
        .ad2_pin = GRAY_SENSOR_GPIO_AD2_PIN,
        .out_port = GRAY_SENSOR_GPIO_PORT,
        .out_pin = GRAY_SENSOR_GPIO_OUT_PIN,
        .active_state = ChassisLineTraceGrayActiveState(),
        .channel_order = GRAY_SENSOR_CHANNEL_1_ON_LEFT,
        .settle_time_us = CHASSIS_LINE_TRACE_GRAY_SETTLE_TIME_US,
        .id = NULL,
    };
    line_trace_sensor = GraySensorRegister(&gray_config);

    chassis_line_trace_init_status =
        (key_ok && (line_trace_sensor != NULL)) ? DEVICE_OK : DEVICE_ERROR;

    if (chassis_line_trace_init_status == DEVICE_OK) {
        line_trace_key1_was_pressed =
            ChassisLineTraceKeyPressed(&line_trace_key1);
        line_trace_key3_was_pressed =
            ChassisLineTraceKeyPressed(&line_trace_key3);
    }
}

static void ChassisLineTraceTask(void)
{
    if (chassis_line_trace_init_status != DEVICE_OK) {
        return;
    }

    bool key1_pressed = ChassisLineTraceKeyPressed(&line_trace_key1);
    bool key3_pressed = ChassisLineTraceKeyPressed(&line_trace_key3);

    if (key1_pressed && !line_trace_key1_was_pressed) {
        chassis_line_trace_key1_press_count++;
        ChassisLineTraceStart();
    }
    if (key3_pressed && !line_trace_key3_was_pressed) {
        chassis_line_trace_key3_press_count++;
        ChassisLineTraceStop(CHASSIS_LINE_TRACE_IDLE);
    }

    line_trace_key1_was_pressed = key1_pressed;
    line_trace_key3_was_pressed = key3_pressed;

    if (chassis_line_trace_state != CHASSIS_LINE_TRACE_RUNNING) {
        return;
    }

    line_trace_sensor->active_state = ChassisLineTraceGrayActiveState();
    if (GraySensorUpdate(line_trace_sensor) != DEVICE_OK) {
        ChassisLineTraceStop(CHASSIS_LINE_TRACE_ERROR);
        return;
    }

    chassis_line_trace_raw = GraySensorGetRawValue(line_trace_sensor);
    chassis_line_trace_active_count = line_trace_sensor->active_count;
    chassis_line_trace_offset = line_trace_sensor->offset;

    uint32_t threshold = chassis_line_trace_stop_active_threshold;
    if (threshold == 0U) {
        threshold = CHASSIS_LINE_TRACE_DEFAULT_STOP_ACTIVE_COUNT;
    }
    if (threshold > GRAY_SENSOR_CHANNEL_COUNT) {
        threshold = GRAY_SENSOR_CHANNEL_COUNT;
    }

    bool on_long_black_line = line_trace_sensor->active_count >= threshold;
    if (on_long_black_line && !line_trace_cross_latched) {
        chassis_line_trace_cross_count++;
        line_trace_cross_latched = true;
    } else if (!on_long_black_line) {
        line_trace_cross_latched = false;
    }

    if (chassis_line_trace_cross_count >= 2U) {
        ChassisLineTraceStop(CHASSIS_LINE_TRACE_DONE);
        return;
    }

    ChassisSetManualCommand(
        chassis_line_trace_base_vx_mps,
        chassis_line_trace_turn_kp * line_trace_sensor->offset);
}
