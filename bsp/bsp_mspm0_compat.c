#include "bsp_mspm0_compat.h"
#include <string.h>

#define MSPM0_UART_TX_BUFFER_SIZE 256U

static UART_HandleTypeDef *active_tx_handle;
static uint8_t tx_buffer[MSPM0_UART_TX_BUFFER_SIZE];
static uint16_t tx_size;
static volatile uint16_t tx_index;
static bool tx_dma_active;

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
    if ((huart == NULL) || (huart->Instance == NULL) ||
        (pData == NULL) || (Size == 0U) ||
        (Size > MSPM0_UART_TX_BUFFER_SIZE)) {
        return HAL_ERROR;
    }

    if (active_tx_handle != NULL) {
        return HAL_BUSY;
    }

    memcpy(tx_buffer, pData, Size);
    tx_size = Size;
    tx_index = 0U;
    tx_dma_active = false;
    active_tx_handle = huart;
    huart->gState |= HAL_UART_STATE_BUSY_TX;

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    DL_UART_Main_enableInterrupt(UART_0_INST,
        DL_UART_MAIN_INTERRUPT_TX);

    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit_DMA(
    UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    if ((huart == NULL) || (huart->Instance == NULL) ||
        (pData == NULL) || (Size == 0U) ||
        (Size > MSPM0_UART_TX_BUFFER_SIZE)) {
        return HAL_ERROR;
    }

    if (active_tx_handle != NULL) {
        return HAL_BUSY;
    }

    memcpy(tx_buffer, pData, Size);
    tx_size = Size;
    tx_index = 0U;
    tx_dma_active = true;
    active_tx_handle = huart;
    huart->gState |= HAL_UART_STATE_BUSY_TX;

    DL_UART_Main_disableInterrupt(UART_0_INST,
        DL_UART_MAIN_INTERRUPT_TX);
    DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
    DL_DMA_setSrcAddr(DMA, DMA_CH1_CHAN_ID,
        (uint32_t) &tx_buffer[0]);
    DL_DMA_setDestAddr(DMA, DMA_CH1_CHAN_ID,
        (uint32_t) &UART_0_INST->TXDATA);
    DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, Size);

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);

    return HAL_OK;
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
    huart->gState |= HAL_UART_STATE_BUSY_RX;

    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t) &UART_0_INST->RXDATA);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t) pData);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, Size);

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    return HAL_OK;
}

void BSP_MSPM0_UART0_IRQHandler(void)
{
    UART_Regs *uart = UART_0_INST;
    DL_UART_IIDX pending = DL_UART_Main_getPendingInterrupt(uart);

    if (pending == DL_UART_IIDX_RX) {
        while (!DL_UART_Main_isRXFIFOEmpty(uart)) {
            if ((huart0.rx_buffer == NULL) ||
                (huart0.rx_count >= huart0.rx_size)) {
                (void) DL_UART_Main_receiveData(uart);
                huart0.gState &= ~HAL_UART_STATE_BUSY_RX;
                HAL_UART_ErrorCallback(&huart0);
                return;
            }

            huart0.rx_buffer[huart0.rx_count++] =
                DL_UART_Main_receiveData(uart);
        }
        return;
    }

    if (pending == DL_UART_IIDX_RX_TIMEOUT_ERROR) {
        DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
        uint16_t remaining =
            DL_DMA_getTransferSize(DMA, DMA_CH0_CHAN_ID);
        uint16_t received = (remaining <= huart0.rx_size)
                                ? (uint16_t) (huart0.rx_size - remaining)
                                : 0U;
        huart0.rx_count = received;

        if (received != 0U) {
            huart0.gState &= ~HAL_UART_STATE_BUSY_RX;
            HAL_UARTEx_RxEventCallback(&huart0, received);
        } else {
            DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        }
        return;
    }

    if (pending == DL_UART_IIDX_DMA_DONE_RX) {
        huart0.rx_count = huart0.rx_size;
        huart0.gState &= ~HAL_UART_STATE_BUSY_RX;
        HAL_UARTEx_RxEventCallback(&huart0, huart0.rx_size);
        return;
    }

    if (pending == DL_UART_IIDX_DMA_DONE_TX) {
        DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
        return;
    }

    if (pending == DL_UART_IIDX_EOT_DONE) {
        if (tx_dma_active && (active_tx_handle != NULL)) {
            active_tx_handle->gState &= ~HAL_UART_STATE_BUSY_TX;
            active_tx_handle = NULL;
            tx_dma_active = false;
            tx_size = 0U;
            tx_index = 0U;
        }
        return;
    }

    if (pending == DL_UART_IIDX_TX) {
        while ((tx_index < tx_size) &&
               !DL_UART_Main_isTXFIFOFull(uart)) {
            DL_UART_Main_transmitData(uart, tx_buffer[tx_index++]);
        }

        if (tx_index >= tx_size) {
            DL_UART_Main_disableInterrupt(uart,
                DL_UART_MAIN_INTERRUPT_TX);
            if (active_tx_handle != NULL) {
                active_tx_handle->gState &= ~HAL_UART_STATE_BUSY_TX;
            }
            active_tx_handle = NULL;
            tx_dma_active = false;
            tx_size = 0U;
            tx_index = 0U;
        }
        return;
    }

    if ((pending == DL_UART_IIDX_OVERRUN_ERROR) ||
        (pending == DL_UART_IIDX_BREAK_ERROR) ||
        (pending == DL_UART_IIDX_PARITY_ERROR) ||
        (pending == DL_UART_IIDX_FRAMING_ERROR) ||
        (pending == DL_UART_IIDX_NOISE_ERROR)) {
        HAL_UART_ErrorCallback(&huart0);
    }
}

void UART0_IRQHandler(void)
{
    BSP_MSPM0_UART0_IRQHandler();
}
