#include "chassis.h"

#include <stddef.h>
#include <stdint.h>

#include "motor.h"
#include "robot_def.h"
#include "ti_msp_dl_config.h"

#define CHASSIS_MOTOR_COUNT          2U
#define CHASSIS_MOTOR_PWM_PERIOD_SEC 0.00005f

Motor_Device_t chassis_motors[CHASSIS_MOTOR_COUNT];

volatile bool chassis_debug_enabled;
volatile float chassis_debug_left_output;
volatile float chassis_debug_right_output;

static bool g_initialized;

static bool ChassisInitMotor(Motor_Device_t *motor,
    TIM_HandleTypeDef *pwm_handle, GPIO_TypeDef *phase_port,
    uint32_t phase_pin, Encoder_Device_t *encoder, bool driver_reverse,
    bool encoder_reverse)
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
                .reverse = driver_reverse,
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
        .encoder_reverse = encoder_reverse,
    };
    return Motor_Init(motor, &config);
}

static void ChassisStop(void)
{
    for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
        Motor_SetTargetSpeed(&chassis_motors[i], 0.0f);
        Motor_Stop(&chassis_motors[i]);
    }
}

static void ChassisSetWheelTargets(const Chassis_Command_t *command)
{
    const float half_track = CHASSIS_TRACK_WIDTH_M * 0.5f;
    const float left_mps = command->vx_mps - command->wz_radps * half_track;
    const float right_mps = command->vx_mps + command->wz_radps * half_track;
    const float counts_per_meter =
        (CHASSIS_ENCODER_PPR * CHASSIS_ENCODER_QUADRATURE *
            CHASSIS_MOTOR_GEAR_RATIO) /
        (2.0f * 3.14159265f * CHASSIS_WHEEL_RADIUS_M);

    Motor_SetTargetSpeed(&chassis_motors[0], left_mps * counts_per_meter);
    Motor_SetTargetSpeed(&chassis_motors[1], right_mps * counts_per_meter);
}

static void ChassisWriteStatus(Chassis_Status_t *status, bool enabled)
{
    chassis_debug_enabled = enabled;
    chassis_debug_left_output = chassis_motors[0].control_output;
    chassis_debug_right_output = chassis_motors[1].control_output;

    if (status == NULL) {
        return;
    }

    status->left_target_counts_s = chassis_motors[0].target_speed;
    status->left_measured_counts_s = chassis_motors[0].measured_speed;
    status->right_target_counts_s = chassis_motors[1].target_speed;
    status->right_measured_counts_s = chassis_motors[1].measured_speed;
    status->enabled = enabled;
}

bool ChassisInit(void)
{
    if (g_initialized) {
        return true;
    }

    if (!ChassisInitMotor(&chassis_motors[0], &htim1,
        MOTOR_GPIO_AIN1_PORT, MOTOR_GPIO_AIN1_PIN, &hencoder_left,
        (bool) CHASSIS_LEFT_DRIVER_REVERSE,
        (bool) CHASSIS_LEFT_ENCODER_REVERSE)) {
        return false;
    }
    if (!ChassisInitMotor(&chassis_motors[1], &htim2,
        MOTOR_GPIO_BIN1_PORT, MOTOR_GPIO_BIN1_PIN, &hencoder_right,
        (bool) CHASSIS_RIGHT_DRIVER_REVERSE,
        (bool) CHASSIS_RIGHT_ENCODER_REVERSE)) {
        ChassisStop();
        return false;
    }

    ChassisStop();
    g_initialized = true;
    return true;
}

void ChassisTask(const Chassis_Command_t *command,
    float dt_seconds, Chassis_Status_t *status)
{
    if (!g_initialized || (command == NULL) || !command->enabled) {
        if (g_initialized) {
            ChassisStop();
        }
        ChassisWriteStatus(status, false);
        return;
    }

    for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
        if (!chassis_motors[i].enabled) {
            Motor_Enable(&chassis_motors[i]);
        }
    }

    ChassisSetWheelTargets(command);
    if (dt_seconds > 0.0f) {
        for (uint8_t i = 0U; i < CHASSIS_MOTOR_COUNT; i++) {
            (void) Motor_Update(&chassis_motors[i], dt_seconds);
        }
    }
    ChassisWriteStatus(status, true);
}
