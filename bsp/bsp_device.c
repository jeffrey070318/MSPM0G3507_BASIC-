#include "bsp_device.h"

UART_HandleTypeDef huart1 = {
    .Instance = UART_0_INST,
};

I2C_HandleTypeDef hi2c1 = {
    .Instance = I2C_0_OLED_INST,
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
    .tclk_hz = MOTOR_PWM_INST_CLK_FREQ,
    .period_ticks = 4000U,
};

TIM_HandleTypeDef htim2 = {
    .Instance = MOTOR_PWM_INST,
    .tclk_hz = MOTOR_PWM_INST_CLK_FREQ,
    .period_ticks = 4000U,
};

TIM_HandleTypeDef htim3 = {
    .Instance = SERVO_PWM_1_INST,
    .tclk_hz = SERVO_PWM_1_INST_CLK_FREQ,
    .period_ticks = 20000U,
};

TIM_HandleTypeDef htim4 = {
    .Instance = SERVO_PWM_2_INST,
    .tclk_hz = SERVO_PWM_2_INST_CLK_FREQ,
    .period_ticks = 20000U,
};
