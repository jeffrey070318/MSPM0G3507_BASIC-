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
    .period_ticks = 1000U,
};

TIM_HandleTypeDef htim5 = {
    .Instance = CAPTURE_0_INST,
    .tclk_hz = 80000000U,
    .period_ticks = CAPTURE_0_INST_LOAD_VALUE + 1U,
};

TIM_HandleTypeDef htim6 = {
    .Instance = CAPTURE_1_INST,
    .tclk_hz = 40000000U,
    .period_ticks = CAPTURE_1_INST_LOAD_VALUE + 1U,
};