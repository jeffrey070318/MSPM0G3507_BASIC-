#include "bsp_device.h"

UART_HandleTypeDef huart1 = {
    .Instance = UART1_INST,
};

UART_HandleTypeDef huart2 = {
    .Instance = UART2_INST,
};

UART_HandleTypeDef huart3 = {
    .Instance = UART3_INST,
};

I2C_HandleTypeDef hi2c1 = {
    .Instance = OLED_I2C_INST,
};

I2C_HandleTypeDef hi2c2 = {
    .Instance = MPU_I2C_INST,
};

SPI_HandleTypeDef hspi1 = {
#ifdef SPI_0_INST
    .Instance = SPI_0_INST,
#else
    .Instance = NULL,
#endif
};

TIM_HandleTypeDef htim1 = {
    .Instance = MOTOR_PWM_INST,
    .Channel = GPIO_MOTOR_PWM_C0_IDX,
    .tclk_hz = MOTOR_PWM_INST_CLK_FREQ,
    .period_ticks = 4000U,
    .count_up = false,
};

TIM_HandleTypeDef htim2 = {
    .Instance = MOTOR_PWM_INST,
    .Channel = GPIO_MOTOR_PWM_C1_IDX,
    .tclk_hz = MOTOR_PWM_INST_CLK_FREQ,
    .period_ticks = 4000U,
    .count_up = false,
};

TIM_HandleTypeDef htim3 = {
    .Instance = SERVO_PWM_1_INST,
    .Channel = GPIO_SERVO_PWM_1_C0_IDX,
    .tclk_hz = SERVO_PWM_1_INST_CLK_FREQ,
    .period_ticks = 20000U,
    .count_up = true,
};

TIM_HandleTypeDef htim4 = {
    .Instance = SERVO_PWM_2_INST,
    .Channel = GPIO_SERVO_PWM_2_C0_IDX,
    .tclk_hz = SERVO_PWM_2_INST_CLK_FREQ,
    .period_ticks = 20000U,
    .count_up = true,
};
