#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chassis.h"
#include "motor.h"
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

float DWT_GetDeltaT(uint32_t *last_tick)
{
    (*last_tick)++;
    return 0.005f;
}

int main(void)
{
    ChassisInit();

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

    assert(captured_configs[1].driver.type == MOTOR_DRIVER_DRV8701E);
    assert(captured_configs[1].driver.config.drv8701e.pwm_handle == &htim2);
    assert(captured_configs[1].driver.config.drv8701e.phase_port ==
        MOTOR_GPIO_BIN1_PORT);
    assert(captured_configs[1].driver.config.drv8701e.phase_pin ==
        MOTOR_GPIO_BIN1_PIN);
    assert(captured_configs[1].encoder == &hencoder_right);
    assert(captured_configs[1].driver.config.drv8701e.reverse ==
        captured_configs[1].encoder_reverse);

    ChassisTask();
    assert(motor_stop_count[0] == 1U);
    assert(motor_stop_count[1] == 1U);

    ChassisSetManualCommand(0.2f, 0.1f);
    assert(chassis_manual_enabled);
    assert(chassis_manual_vx_mps == 0.2f);
    assert(chassis_manual_wz_radps == 0.1f);
    ChassisTask();
    assert(motor_enable_count[0] == 1U);
    assert(motor_enable_count[1] == 1U);
    assert(motor_update_count[0] == 1U);
    assert(motor_update_count[1] == 1U);
    assert(registered_motors[0]->target_speed > 0.0f);
    assert(registered_motors[1]->target_speed >
        registered_motors[0]->target_speed);

    ChassisDisableManualCommand();
    assert(!chassis_manual_enabled);
    ChassisTask();
    assert(motor_stop_count[0] == 2U);
    assert(motor_stop_count[1] == 2U);
    return 0;
}
