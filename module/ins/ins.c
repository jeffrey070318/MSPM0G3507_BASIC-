#include "ins.h"

#include <stddef.h>
#include <string.h>

#include "imu.h"
#include "message_center.h"
#include "pid.h"

#define INS_PI               (3.14159265359f)
#define INS_HALF_PI          (1.57079632679f)
#define INS_DEG_TO_RAD       (0.01745329252f)
#define INS_RAD_TO_DEG       (57.2957795131f)

#define INS_DIST_KP          (0.3f)
#define INS_DIST_KI          (0.015f)
#define INS_DIST_KD          (0.0f)
#define INS_DIST_MAX_OUT     (0.4f)
#define INS_DIST_MAX_IOUT    (0.3f)

#define INS_ANGLE_KP         (2.5f)
#define INS_ANGLE_KI         (0.03f)
#define INS_ANGLE_KD         (0.0f)
#define INS_ANGLE_MAX_OUT    (2.0f)
#define INS_ANGLE_MAX_IOUT   (1.5f)

static Subscriber_t *g_imu_subscriber;
static Subscriber_t *g_encoder_subscriber;
static Publisher_t *g_command_publisher;

static INS_Position_t g_position;
static INS_State_e g_state;
static float g_yaw_deg;
static float g_origin_yaw_deg;
static int32_t g_last_left_count;
static int32_t g_last_right_count;
static bool g_heading_reference_valid;
static bool g_encoder_reference_valid;
static bool g_initialized;

static PID_Controller_t g_distance_pid;
static PID_Controller_t g_angle_pid;

static float INSAbsolute(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float INSNormalizeDegrees(float angle)
{
    while (angle > 180.0f) {
        angle -= 360.0f;
    }
    while (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}

static float INSNormalizeRadians(float angle)
{
    while (angle > INS_PI) {
        angle -= 2.0f * INS_PI;
    }
    while (angle < -INS_PI) {
        angle += 2.0f * INS_PI;
    }
    return angle;
}

static float INSSin(float angle)
{
    float x = INSNormalizeRadians(angle);
    if (x > INS_HALF_PI) {
        x = INS_PI - x;
    } else if (x < -INS_HALF_PI) {
        x = -INS_PI - x;
    }

    float x2 = x * x;
    return x * (1.0f - x2 * (0.16666667f -
        x2 * (0.008333333f - x2 * 0.000198413f)));
}

static float INSCos(float angle)
{
    return INSSin(angle + INS_HALF_PI);
}

static float INSAtanUnit(float value)
{
    float absolute = INSAbsolute(value);
    return INS_HALF_PI * 0.5f * value -
        value * (absolute - 1.0f) *
            (0.2447f + 0.0663f * absolute);
}

static float INSAtan2(float y, float x)
{
    if (x == 0.0f) {
        if (y > 0.0f) {
            return INS_HALF_PI;
        }
        if (y < 0.0f) {
            return -INS_HALF_PI;
        }
        return 0.0f;
    }

    float ratio = y / x;
    float angle;
    if (INSAbsolute(ratio) <= 1.0f) {
        angle = INSAtanUnit(ratio);
        if ((x < 0.0f) && (y >= 0.0f)) {
            angle += INS_PI;
        } else if ((x < 0.0f) && (y < 0.0f)) {
            angle -= INS_PI;
        }
    } else {
        float reciprocal_angle = INSAtanUnit(x / y);
        angle = (y > 0.0f)
            ? (INS_HALF_PI - reciprocal_angle)
            : (-INS_HALF_PI - reciprocal_angle);
    }
    return INSNormalizeRadians(angle);
}

static float INSSquareRoot(float value)
{
    if (value <= 0.0f) {
        return 0.0f;
    }

    float estimate = (value > 1.0f) ? value : 1.0f;
    for (uint8_t i = 0U; i < 8U; ++i) {
        estimate = 0.5f * (estimate + value / estimate);
    }
    return estimate;
}

static float INSDistanceFromOrigin(void)
{
    return INSSquareRoot(
        g_position.x * g_position.x + g_position.y * g_position.y);
}

bool INS_Init(void)
{
    if (g_initialized) {
        return true;
    }

    if (g_imu_subscriber == NULL) {
        g_imu_subscriber =
            SubRegister(INS_IMU_TOPIC, sizeof(IMU_Data_t));
    }
    if (g_encoder_subscriber == NULL) {
        g_encoder_subscriber = SubRegister(
            INS_ENCODER_TOPIC, sizeof(Encoder_Pub_Data_t));
    }
    if (g_command_publisher == NULL) {
        g_command_publisher = PubRegister(
            INS_CMD_TOPIC, sizeof(INS_ChassisCommand_t));
    }
    if ((g_imu_subscriber == NULL) ||
        (g_encoder_subscriber == NULL) ||
        (g_command_publisher == NULL)) {
        return false;
    }

    PID_Config_t distance_config = {
        .kp = INS_DIST_KP,
        .ki = INS_DIST_KI,
        .kd = INS_DIST_KD,
        .output_limit = INS_DIST_MAX_OUT,
        .integral_limit = INS_DIST_MAX_IOUT,
        .deadband = 0.0f,
        .derivative_on_measurement = false,
    };
    PID_Config_t angle_config = {
        .kp = INS_ANGLE_KP,
        .ki = INS_ANGLE_KI,
        .kd = INS_ANGLE_KD,
        .output_limit = INS_ANGLE_MAX_OUT,
        .integral_limit = INS_ANGLE_MAX_IOUT,
        .deadband = 0.0f,
        .derivative_on_measurement = false,
    };
    if (!PID_ControllerInit(&g_distance_pid, &distance_config) ||
        !PID_ControllerInit(&g_angle_pid, &angle_config)) {
        return false;
    }

    memset(&g_position, 0, sizeof(g_position));
    g_state = INS_STATE_IDLE;
    g_yaw_deg = 0.0f;
    g_origin_yaw_deg = 0.0f;
    g_heading_reference_valid = false;
    g_encoder_reference_valid = false;
    g_initialized = true;
    return true;
}

void INS_Task(float dt_seconds)
{
    if (!g_initialized) {
        return;
    }

    IMU_Data_t imu_data;
    if (SubGetMessage(g_imu_subscriber, &imu_data) != 0U) {
        g_yaw_deg = imu_data.yaw;
        if (!g_heading_reference_valid) {
            g_origin_yaw_deg = g_yaw_deg;
            g_heading_reference_valid = true;
        }
    }

    Encoder_Pub_Data_t encoder_data;
    if (SubGetMessage(g_encoder_subscriber, &encoder_data) != 0U) {
        if (!g_encoder_reference_valid) {
            g_last_left_count = encoder_data.left_total;
            g_last_right_count = encoder_data.right_total;
            g_encoder_reference_valid = true;
        } else {
            int32_t left_delta =
                encoder_data.left_total - g_last_left_count;
            int32_t right_delta =
                encoder_data.right_total - g_last_right_count;
            g_last_left_count = encoder_data.left_total;
            g_last_right_count = encoder_data.right_total;

            if (g_heading_reference_valid) {
                float distance = (float) (left_delta + right_delta) *
                    0.5f * INS_METERS_PER_COUNT;
                float heading = INSNormalizeDegrees(
                    g_yaw_deg - g_origin_yaw_deg) * INS_DEG_TO_RAD;
                g_position.x += distance * INSCos(heading);
                g_position.y += distance * INSSin(heading);
            }
        }
    }

    INS_ChassisCommand_t command = {0};
    if ((g_state == INS_STATE_RETURNING) &&
        g_heading_reference_valid && (dt_seconds > 0.0f)) {
        float distance = INSDistanceFromOrigin();
        if (distance <= INS_TARGET_DISTANCE_M) {
            g_state = INS_STATE_DONE;
            PID_ControllerReset(&g_distance_pid);
            PID_ControllerReset(&g_angle_pid);
        } else {
            float target_heading = INSAtan2(
                -g_position.y, -g_position.x) * INS_RAD_TO_DEG;
            float current_heading = INSNormalizeDegrees(
                g_yaw_deg - g_origin_yaw_deg);
            float heading_error = INSNormalizeDegrees(
                target_heading - current_heading);
            float alignment = INSCos(heading_error * INS_DEG_TO_RAD);
            if (alignment < 0.0f) {
                alignment = 0.0f;
            }

            command.vx = PID_ControllerUpdate(
                &g_distance_pid, distance, 0.0f, dt_seconds) * alignment;
            command.wz = PID_ControllerUpdate(
                &g_angle_pid, heading_error, 0.0f, dt_seconds);
            command.motion_enabled = true;
        }
    }

    (void) PubPushMessage(g_command_publisher, &command);
}

void INS_StartReturn(void)
{
    if (!g_initialized) {
        return;
    }
    PID_ControllerReset(&g_distance_pid);
    PID_ControllerReset(&g_angle_pid);
    g_state = INS_STATE_RETURNING;
}

void INS_ResetOrigin(void)
{
    if (!g_initialized) {
        return;
    }
    memset(&g_position, 0, sizeof(g_position));
    if (g_heading_reference_valid) {
        g_origin_yaw_deg = g_yaw_deg;
    }
    g_encoder_reference_valid = false;
    PID_ControllerReset(&g_distance_pid);
    PID_ControllerReset(&g_angle_pid);
    g_state = INS_STATE_IDLE;
}

INS_Position_t INS_GetPosition(void)
{
    return g_position;
}

INS_State_e INS_GetState(void)
{
    return g_state;
}

INS_TargetState_e INS_CheckTargetReach(void)
{
    float distance = INSDistanceFromOrigin();
    if (distance <= INS_TARGET_DISTANCE_M) {
        return INS_TARGET_REACHED;
    }
    if (distance <= INS_TARGET_DISTANCE_M * 6.0f) {
        return INS_TARGET_APPROACHING;
    }
    return INS_TARGET_FAR;
}
