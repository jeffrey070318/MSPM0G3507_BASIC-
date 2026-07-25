#include "hardware_test_task.h"

#if HARDWARE_TEST_MODE == HARDWARE_TEST_MOTOR

#include <stdbool.h>
#include <stdint.h>

#include "hardware_test_motor_sequence.h"
#include "motor.h"
#include "ti_msp_dl_config.h"

#define MOTOR_TEST_PERIOD_SECONDS 0.01f
#define MOTOR_TEST_TARGET_COUNTS_PER_SECOND 1500.0f
#define MOTOR_TEST_KP                       0.0002f
#define MOTOR_TEST_KI                       0.0005f
#define MOTOR_TEST_KD                       0.0f
#define MOTOR_TEST_OUTPUT_LIMIT             0.5f

volatile HardwareTestMotorStage_e hardware_test_motor_stage;
volatile float hardware_test_motor_output;
volatile float hardware_test_motor_target_counts_per_second =
    MOTOR_TEST_TARGET_COUNTS_PER_SECOND;
volatile float hardware_test_motor_pid_error;
volatile float hardware_test_motor_pid_integral;
volatile bool hardware_test_motor_initialized;
volatile int32_t hardware_test_motor_encoder_total;
volatile int16_t hardware_test_motor_encoder_delta;
volatile float hardware_test_motor_encoder_counts_per_second;
volatile uint32_t hardware_test_motor_encoder_invalid_count;
volatile float hardware_test_motor_pwm_duty;
volatile bool hardware_test_motor_pwm_running;
volatile bool hardware_test_motor_timer_running;
volatile uint32_t hardware_test_motor_timer_load;
volatile uint32_t hardware_test_motor_timer_compare;
volatile uint32_t hardware_test_motor_timer_count;
volatile bool hardware_test_motor_phase_high;

Motor_Device_t hardware_test_motor;
static HardwareTestMotorSequence_t hardware_test_motor_sequence;

static void HardwareTestMotorUpdateTelemetry(void)
{
    DRV8701E_Driver_t *driver =
        &hardware_test_motor.driver.context.drv8701e;

    hardware_test_motor_stage = hardware_test_motor_sequence.stage;
    hardware_test_motor_output = hardware_test_motor.control_output;
    hardware_test_motor_pid_error = hardware_test_motor.speed_pid.error;
    hardware_test_motor_pid_integral = hardware_test_motor.speed_pid.integral;
    hardware_test_motor_encoder_total =
        Encoder_Get_Total(&hencoder_left);
    hardware_test_motor_encoder_delta =
        Encoder_Get_Speed(&hencoder_left);
    hardware_test_motor_encoder_counts_per_second =
        (float) hardware_test_motor_encoder_delta /
        MOTOR_TEST_PERIOD_SECONDS;
    hardware_test_motor_encoder_invalid_count =
        Encoder_Get_InvalidTransitions(&hencoder_left);

    if ((driver->enable_pwm != NULL) &&
        (driver->enable_pwm->htim != NULL) &&
        (driver->enable_pwm->htim->Instance != NULL)) {
        GPTIMER_Regs *timer = driver->enable_pwm->htim->Instance;
        hardware_test_motor_pwm_duty = driver->enable_pwm->dutyratio;
        hardware_test_motor_pwm_running = driver->enable_pwm->running;
        hardware_test_motor_timer_running = DL_Timer_isRunning(timer);
        hardware_test_motor_timer_load = DL_Timer_getLoadValue(timer);
        hardware_test_motor_timer_compare =
            DL_Timer_getCaptureCompareValue(timer,
                (DL_TIMER_CC_INDEX) driver->enable_pwm->channel);
        hardware_test_motor_timer_count = DL_Timer_getTimerCount(timer);
    } else {
        hardware_test_motor_pwm_duty = 0.0f;
        hardware_test_motor_pwm_running = false;
        hardware_test_motor_timer_running = false;
        hardware_test_motor_timer_load = 0U;
        hardware_test_motor_timer_compare = 0U;
        hardware_test_motor_timer_count = 0U;
    }

    hardware_test_motor_phase_high =
        (driver->phase != NULL) &&
        ((driver->phase->GPIOx->DOUT31_0 & driver->phase->GPIO_Pin) != 0U);
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
            .kp = MOTOR_TEST_KP,
            .ki = MOTOR_TEST_KI,
            .kd = MOTOR_TEST_KD,
            .output_limit = MOTOR_TEST_OUTPUT_LIMIT,
            .integral_limit = MOTOR_TEST_OUTPUT_LIMIT,
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
    } else if (hardware_test_motor_sequence.stage ==
               HARDWARE_TEST_MOTOR_FORWARD) {
        Motor_SetTargetSpeed(&hardware_test_motor,
            hardware_test_motor_target_counts_per_second);
    } else {
        Motor_SetOpenLoop(&hardware_test_motor, 0.0f);
    }

    (void) Motor_Update(
        &hardware_test_motor, MOTOR_TEST_PERIOD_SECONDS);
    HardwareTestMotorUpdateTelemetry();
}

#endif
