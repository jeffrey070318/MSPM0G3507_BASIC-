#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_MOTOR

#include <stdbool.h>
#include <stdint.h>

#include "hardware_test_motor_sequence.h"
#include "motor.h"
#include "ti_msp_dl_config.h"

#define MOTOR_TEST_PERIOD_SECONDS 0.01f

volatile HardwareTestMotorStage_e hardware_test_motor_stage;
volatile float hardware_test_motor_output;
volatile bool hardware_test_motor_initialized;
volatile int32_t hardware_test_motor_encoder_total;
volatile int16_t hardware_test_motor_encoder_delta;
volatile float hardware_test_motor_encoder_counts_per_second;
volatile uint32_t hardware_test_motor_encoder_invalid_count;

static Motor_Device_t hardware_test_motor;
static HardwareTestMotorSequence_t hardware_test_motor_sequence;

static void HardwareTestMotorUpdateTelemetry(void)
{
    hardware_test_motor_stage = hardware_test_motor_sequence.stage;
    hardware_test_motor_output = hardware_test_motor_sequence.output;
    hardware_test_motor_encoder_total =
        Encoder_Get_Total(&hencoder_left);
    hardware_test_motor_encoder_delta =
        Encoder_Get_Speed(&hencoder_left);
    hardware_test_motor_encoder_counts_per_second =
        (float) hardware_test_motor_encoder_delta /
        MOTOR_TEST_PERIOD_SECONDS;
    hardware_test_motor_encoder_invalid_count =
        Encoder_Get_InvalidTransitions(&hencoder_left);
}

Device_Status_e HardwareTestInit(void)
{
    Motor_Init_Config_t config = {
        .driver = {
            .type = MOTOR_DRIVER_DRV8701E,
            .config.drv8701e = {
                .pwm_handle = &htim1,
                .pwm_channel = htim1.Channel,
                .pwm_period = 0.00005f,
                .phase_port = MOTOR_GPIO_AIN1_PORT,
                .phase_pin = MOTOR_GPIO_AIN1_PIN,
                .reverse = false,
            },
        },
        .encoder = &hencoder_left,
        .speed_pid = {
            .kp = 0.001f,
            .ki = 0.0f,
            .kd = 0.0f,
            .output_limit = 1.0f,
            .integral_limit = 0.5f,
            .deadband = 0.0f,
            .derivative_on_measurement = true,
        },
        .encoder_reverse = false,
    };

    HardwareTestMotorSequence_Init(&hardware_test_motor_sequence);
    hardware_test_motor_initialized =
        Motor_Init(&hardware_test_motor, &config);
    if (!hardware_test_motor_initialized) {
        hardware_test_motor_sequence.stage =
            HARDWARE_TEST_MOTOR_COMPLETE;
        hardware_test_motor_sequence.output = 0.0f;
        hardware_test_motor_sequence.stop_latched = true;
        HardwareTestMotorUpdateTelemetry();
        return DEVICE_ERROR;
    }

    Motor_SetOpenLoop(&hardware_test_motor, 0.0f);
    HardwareTestMotorUpdateTelemetry();
    return DEVICE_OK;
}

void HardwareTestRun(void)
{
    if (!hardware_test_motor_initialized) {
        return;
    }

    HardwareTestMotorSequence_Step(&hardware_test_motor_sequence);
    if (hardware_test_motor_sequence.stop_latched) {
        if (hardware_test_motor.enabled) {
            Motor_Stop(&hardware_test_motor);
        }
    } else {
        Motor_SetOpenLoop(
            &hardware_test_motor, hardware_test_motor_sequence.output);
    }

    (void) Motor_Update(
        &hardware_test_motor, MOTOR_TEST_PERIOD_SECONDS);
    HardwareTestMotorUpdateTelemetry();
}

#endif
