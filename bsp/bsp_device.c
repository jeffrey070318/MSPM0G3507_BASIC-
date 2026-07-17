#include "bsp_device.h"

UART_HandleTypeDef huart1 = {
    .Instance = UART_0_INST,
};

I2C_HandleTypeDef hi2c1 = {
    .Instance = I2C_0_INST,
};

SPI_HandleTypeDef hspi1 = {
    .Instance = SPI_0_INST,
};

TIM_HandleTypeDef htim1 = {
    .Instance = PWM_0_INST,
    .tclk_hz = PWM_0_INST_CLK_FREQ,
    .period_ticks = 4000U,
};

TIM_HandleTypeDef htim2 = {
    .Instance = PWM_1_INST,
    .tclk_hz = PWM_1_INST_CLK_FREQ,
    .period_ticks = 4000U,
};

TIM_HandleTypeDef htim3 = {
    .Instance = PWM_2_INST,
    .tclk_hz = PWM_2_INST_CLK_FREQ,
    .period_ticks = 2000U,
};

TIM_HandleTypeDef htim4 = {
    .Instance = PWM_3_INST,
    .tclk_hz = PWM_3_INST_CLK_FREQ,
    .period_ticks = 4000U,
};
