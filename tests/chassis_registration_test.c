#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chassis.h"
#include "motor.h"
#include "robot_def.h"
#include "ti_msp_dl_config.h"

extern Motor_Device_t chassis_motors[2];

TIM_HandleTypeDef htim1 = {.Channel = 0U};
TIM_HandleTypeDef htim2 = {.Channel = 1U};
Encoder_Device_t hencoder_left = {.marker = 1U};
Encoder_Device_t hencoder_right = {.marker = 2U};
GPIO_TypeDef test_raw_gpio_a = {.marker = 1U};
GPIO_TypeDef test_raw_gpio_b = {.marker = 2U};
GPIO_TypeDef test_motor_gpio_a = {.marker = 3U};
GPIO_TypeDef test_motor_gpio_b = {.marker = 4U};

static Motor_Init_Config_t captured_configs[2];
static Motor_Device_t *registered_motors[2];
static uint32_t motor_init_count;
static uint32_t motor_stop_count[2];
static uint32_t motor_enable_count[2];
static uint32_t motor_update_count[2];

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static void AssertNear(float actual, float expected)
{
    assert(AbsFloat(actual - expected) < 0.0001f);
}

static size_t MotorIndex(const Motor_Device_t *motor)
{
    assert(motor_init_count == 2U);
    assert(registered_motors[0] == &chassis_motors[0]);
    assert(registered_motors[1] == &chassis_motors[1]);
    if (motor == registered_motors[0]) {
        return 0U;
    }
    assert(motor == registered_motors[1]);
    return 1U;
}

bool Motor_Init(Motor_Device_t *motor, const Motor_Init_Config_t *config)
{
    assert(motor_init_count < 2U);
    captured_configs[motor_init_count] = *config;
    registered_motors[motor_init_count] = motor;
    motor->encoder = config->encoder;
    motor->enabled = true;
    motor_init_count++;
    return true;
}

void Motor_SetTargetSpeed(Motor_Device_t *motor, float counts_per_second)
{
    motor->target_speed = counts_per_second;
}

bool Motor_Update(Motor_Device_t *motor, float dt_seconds)
{
    motor_update_count[MotorIndex(motor)]++;
    return dt_seconds > 0.0f;
}

void Motor_Enable(Motor_Device_t *motor)
{
    motor_enable_count[MotorIndex(motor)]++;
    motor->enabled = true;
}

void Motor_Stop(Motor_Device_t *motor)
{
    motor_stop_count[MotorIndex(motor)]++;
    motor->enabled = false;
}

int main(void)
{
    assert(ChassisInit());

    assert(motor_init_count == 2U);
    assert(captured_configs[0].driver.type == MOTOR_DRIVER_DRV8701E);
    assert(captured_configs[0].driver.config.drv8701e.pwm_handle == &htim1);
    assert(captured_configs[0].driver.config.drv8701e.phase_port ==
        MOTOR_GPIO_AIN1_PORT);
    assert(captured_configs[0].driver.config.drv8701e.phase_pin ==
        MOTOR_GPIO_AIN1_PIN);
    assert(captured_configs[0].encoder == &hencoder_left);
    assert(captured_configs[0].driver.config.drv8701e.reverse ==
        captured_configs[0].encoder_reverse);
    assert(captured_configs[0].encoder_reverse ==
        (bool) CHASSIS_LEFT_MOTOR_REVERSE);
    AssertNear(captured_configs[0].speed_pid.kp, CHASSIS_SPEED_KP);
    AssertNear(captured_configs[0].speed_pid.ki, CHASSIS_SPEED_KI);
    AssertNear(captured_configs[0].speed_pid.kd, CHASSIS_SPEED_KD);
    AssertNear(captured_configs[0].speed_pid.output_limit,
        CHASSIS_SPEED_MAX_OUT);
    AssertNear(captured_configs[0].speed_pid.integral_limit,
        CHASSIS_SPEED_MAX_IOUT);

    assert(captured_configs[1].driver.type == MOTOR_DRIVER_DRV8701E);
    assert(captured_configs[1].driver.config.drv8701e.pwm_handle == &htim2);
    assert(captured_configs[1].driver.config.drv8701e.phase_port ==
        MOTOR_GPIO_BIN1_PORT);
    assert(captured_configs[1].driver.config.drv8701e.phase_pin ==
        MOTOR_GPIO_BIN1_PIN);
    assert(captured_configs[1].encoder == &hencoder_right);
    assert(captured_configs[1].driver.config.drv8701e.reverse ==
        captured_configs[1].encoder_reverse);
    assert(captured_configs[1].encoder_reverse ==
        (bool) CHASSIS_RIGHT_MOTOR_REVERSE);
    assert(motor_stop_count[0] == 1U);
    assert(motor_stop_count[1] == 1U);

    Chassis_Command_t command = {0};
    Chassis_Status_t status = {0};
    ChassisTask(&command, 0.005f, &status);
    assert(motor_stop_count[0] == 2U);
    assert(motor_stop_count[1] == 2U);
    assert(!status.enabled);

    command = (Chassis_Command_t) {
        .vx_mps = 0.2f,
        .wz_radps = 0.1f,
        .enabled = true,
    };
    ChassisTask(&command, 0.005f, &status);
    assert(motor_enable_count[0] == 1U);
    assert(motor_enable_count[1] == 1U);
    assert(motor_update_count[0] == 1U);
    assert(motor_update_count[1] == 1U);
    assert(status.enabled);
    const float counts_per_meter =
        (CHASSIS_ENCODER_PPR * CHASSIS_ENCODER_QUADRATURE *
            CHASSIS_MOTOR_GEAR_RATIO) /
        (2.0f * 3.14159265f * CHASSIS_WHEEL_RADIUS_M);
    AssertNear(registered_motors[0]->target_speed,
        (0.2f - 0.1f * CHASSIS_TRACK_WIDTH_M * 0.5f) *
            counts_per_meter);
    AssertNear(registered_motors[1]->target_speed,
        (0.2f + 0.1f * CHASSIS_TRACK_WIDTH_M * 0.5f) *
            counts_per_meter);

    AssertNear(status.left_target_counts_s,
        registered_motors[0]->target_speed);
    AssertNear(status.right_target_counts_s,
        registered_motors[1]->target_speed);

    command.enabled = false;
    ChassisTask(&command, 0.005f, &status);
    assert(motor_stop_count[0] == 3U);
    assert(motor_stop_count[1] == 3U);
    assert(!status.enabled);
    return 0;
}
