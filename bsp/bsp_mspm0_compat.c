#include "bsp_mspm0_compat.h"

UART_HandleTypeDef huart0 = { .Instance = UART_0_INST,
    .gState = HAL_UART_STATE_READY };
UART_HandleTypeDef huart1 = { .Instance = UART_0_INST,
    .gState = HAL_UART_STATE_READY };
UART_HandleTypeDef huart2 = { .Instance = UART_0_INST,
    .gState = HAL_UART_STATE_READY };

I2C_HandleTypeDef hi2c0 = { .Instance = I2C_0_INST };
I2C_HandleTypeDef hi2c1 = { .Instance = I2C_0_INST };
I2C_HandleTypeDef hi2c2 = { .Instance = I2C_0_INST };

SPI_HandleTypeDef hspi0 = { .Instance = SPI_0_INST };
SPI_HandleTypeDef hspi1 = { .Instance = SPI_0_INST };
SPI_HandleTypeDef hspi2 = { .Instance = SPI_0_INST };

TIM_HandleTypeDef htim1 = { .Instance = PWM_0_INST,
    .Init = { .Prescaler = 0U },
    .tclk_hz = PWM_0_INST_CLK_FREQ,
    .period_ticks = 1000U };
TIM_HandleTypeDef htim2 = { .Instance = PWM_1_INST,
    .Init = { .Prescaler = 0U },
    .tclk_hz = PWM_1_INST_CLK_FREQ,
    .period_ticks = 1000U };
TIM_HandleTypeDef htim3 = { .Instance = PWM_2_INST,
    .Init = { .Prescaler = 0U },
    .tclk_hz = PWM_2_INST_CLK_FREQ,
    .period_ticks = 1000U };
TIM_HandleTypeDef htim4 = { .Instance = PWM_3_INST,
    .Init = { .Prescaler = 0U },
    .tclk_hz = PWM_3_INST_CLK_FREQ,
    .period_ticks = 1000U };

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart,
    uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void) Timeout;

    if ((huart == NULL) || (huart->Instance == NULL) || (pData == NULL)) {
        return HAL_ERROR;
    }

    huart->gState = HAL_UART_STATE_BUSY_TX;
    for (uint16_t i = 0U; i < Size; ++i) {
        DL_UART_Main_transmitDataBlocking(huart->Instance, pData[i]);
    }
    while (DL_UART_Main_isBusy(huart->Instance)) {
    }
    huart->gState = HAL_UART_STATE_READY;

    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(
    UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    return HAL_UART_Transmit(huart, pData, Size, 0U);
}

HAL_StatusTypeDef HAL_UART_Transmit_DMA(
    UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    return HAL_UART_Transmit(huart, pData, Size, 0U);
}

HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(
    UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    if ((huart == NULL) || (huart->Instance == NULL) || (pData == NULL)) {
        return HAL_ERROR;
    }

    huart->rx_buffer = pData;
    huart->rx_size = Size;
    huart->rx_count = 0U;
    huart->gState &= ~HAL_UART_STATE_BUSY_RX;

    return HAL_OK;
}
