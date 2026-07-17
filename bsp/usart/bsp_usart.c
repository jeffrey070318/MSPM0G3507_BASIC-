#include "bsp_usart.h"

#include <stdlib.h>
#include <string.h>

static uint8_t idx;
static USARTInstance *usart_instance[DEVICE_USART_CNT] = {NULL};

static USARTInstance *active_tx_instance;
static uint8_t tx_buffer[USART_TXBUFF_LIMIT];
static uint16_t tx_size;
static volatile uint16_t tx_index;

static void USARTStartReceive(USARTInstance *instance)
{
    if ((instance == NULL) || (instance->usart_handle == NULL) ||
        (instance->usart_handle->Instance != UART_0_INST) ||
        (instance->recv_buff_size == 0U) ||
        (instance->recv_buff_size > USART_RXBUFF_LIMIT)) {
        return;
    }

    instance->recv_count = 0U;
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t) &instance->usart_handle->Instance->RXDATA);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t) instance->recv_buff);
    DL_DMA_setTransferSize(
        DMA, DMA_CH0_CHAN_ID, instance->recv_buff_size);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
}

static void USARTDispatchReceive(USARTInstance *instance, uint16_t size)
{
    if (instance == NULL) {
        return;
    }

    instance->recv_count = size;
    if (instance->module_callback != NULL) {
        instance->module_callback();
    }

    if (size <= USART_RXBUFF_LIMIT) {
        memset(instance->recv_buff, 0, size);
    }
    USARTStartReceive(instance);
}

static Device_Status_e USARTSendBlocking(
    USARTInstance *instance, uint8_t *data, uint16_t size)
{
    if ((instance == NULL) || (instance->usart_handle == NULL) ||
        (instance->usart_handle->Instance == NULL) ||
        (data == NULL) || (size == 0U)) {
        return DEVICE_ERROR;
    }

    instance->tx_busy = 1U;
    for (uint16_t i = 0U; i < size; ++i) {
        DL_UART_Main_transmitDataBlocking(
            instance->usart_handle->Instance, data[i]);
    }
    while (DL_UART_Main_isBusy(instance->usart_handle->Instance)) {
    }
    instance->tx_busy = 0U;
    return DEVICE_OK;
}

static Device_Status_e USARTSendAsync(USARTInstance *instance,
    uint8_t *data, uint16_t size, bool use_dma)
{
    if ((instance == NULL) || (instance->usart_handle == NULL) ||
        (instance->usart_handle->Instance != UART_0_INST) ||
        (data == NULL) || (size == 0U) ||
        (size > USART_TXBUFF_LIMIT)) {
        return DEVICE_ERROR;
    }
    if (active_tx_instance != NULL) {
        return DEVICE_BUSY;
    }

    memcpy(tx_buffer, data, size);
    tx_size = size;
    tx_index = 0U;
    active_tx_instance = instance;
    instance->tx_busy = 1U;

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

    if (use_dma) {
        DL_UART_Main_disableInterrupt(
            UART_0_INST, DL_UART_MAIN_INTERRUPT_TX);
        DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
        DL_DMA_setSrcAddr(
            DMA, DMA_CH1_CHAN_ID, (uint32_t) tx_buffer);
        DL_DMA_setDestAddr(DMA, DMA_CH1_CHAN_ID,
            (uint32_t) &UART_0_INST->TXDATA);
        DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, size);
        DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
    } else {
        DL_UART_Main_enableInterrupt(
            UART_0_INST, DL_UART_MAIN_INTERRUPT_TX);
    }

    return DEVICE_OK;
}

void USARTServiceInit(USARTInstance *_instance)
{
    USARTStartReceive(_instance);
}

USARTInstance *USARTRegister(USART_Init_Config_s *init_config)
{
    if ((init_config == NULL) ||
        (init_config->usart_handle == NULL) ||
        (init_config->usart_handle->Instance != UART_0_INST) ||
        (init_config->recv_buff_size == 0U) ||
        (init_config->recv_buff_size > USART_RXBUFF_LIMIT) ||
        (idx >= DEVICE_USART_CNT)) {
        return NULL;
    }

    for (uint8_t i = 0U; i < idx; ++i) {
        if (usart_instance[i]->usart_handle->Instance ==
            init_config->usart_handle->Instance) {
            return NULL;
        }
    }

    USARTInstance *instance =
        (USARTInstance *) malloc(sizeof(USARTInstance));
    if (instance == NULL) {
        return NULL;
    }
    memset(instance, 0, sizeof(USARTInstance));

    instance->usart_handle = init_config->usart_handle;
    instance->recv_buff_size = init_config->recv_buff_size;
    instance->module_callback = init_config->module_callback;
    usart_instance[idx++] = instance;

    USARTServiceInit(instance);
    return instance;
}

void USARTSend(USARTInstance *_instance, uint8_t *send_buf,
    uint16_t send_size, USART_TRANSFER_MODE mode)
{
    switch (mode) {
    case USART_TRANSFER_BLOCKING:
        (void) USARTSendBlocking(_instance, send_buf, send_size);
        break;
    case USART_TRANSFER_IT:
        (void) USARTSendAsync(_instance, send_buf, send_size, false);
        break;
    case USART_TRANSFER_DMA:
        (void) USARTSendAsync(_instance, send_buf, send_size, true);
        break;
    default:
        break;
    }
}

uint8_t USARTIsReady(USARTInstance *_instance)
{
    return ((_instance != NULL) && (_instance->tx_busy == 0U)) ? 1U : 0U;
}

void USARTIRQHandler(void)
{
    DL_UART_IIDX pending = DL_UART_Main_getPendingInterrupt(UART_0_INST);
    USARTInstance *rx_instance = (idx > 0U) ? usart_instance[0] : NULL;

    if (pending == DL_UART_IIDX_DMA_DONE_RX) {
        if (rx_instance != NULL) {
            USARTDispatchReceive(rx_instance, rx_instance->recv_buff_size);
        }
        return;
    }

    if (pending == DL_UART_IIDX_RX_TIMEOUT_ERROR) {
        if (rx_instance != NULL) {
            DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
            uint16_t remaining =
                DL_DMA_getTransferSize(DMA, DMA_CH0_CHAN_ID);
            uint16_t received =
                (remaining <= rx_instance->recv_buff_size)
                    ? (uint16_t) (rx_instance->recv_buff_size - remaining)
                    : 0U;
            if (received != 0U) {
                USARTDispatchReceive(rx_instance, received);
            } else {
                USARTStartReceive(rx_instance);
            }
        }
        return;
    }

    if (pending == DL_UART_IIDX_DMA_DONE_TX) {
        DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
        return;
    }

    if (pending == DL_UART_IIDX_TX) {
        while ((tx_index < tx_size) &&
               !DL_UART_Main_isTXFIFOFull(UART_0_INST)) {
            DL_UART_Main_transmitData(
                UART_0_INST, tx_buffer[tx_index++]);
        }
        if (tx_index >= tx_size) {
            DL_UART_Main_disableInterrupt(
                UART_0_INST, DL_UART_MAIN_INTERRUPT_TX);
        }
        return;
    }

    if (pending == DL_UART_IIDX_EOT_DONE) {
        if (active_tx_instance != NULL) {
            active_tx_instance->tx_busy = 0U;
        }
        active_tx_instance = NULL;
        tx_size = 0U;
        tx_index = 0U;
        return;
    }

    if ((pending == DL_UART_IIDX_OVERRUN_ERROR) ||
        (pending == DL_UART_IIDX_BREAK_ERROR) ||
        (pending == DL_UART_IIDX_PARITY_ERROR) ||
        (pending == DL_UART_IIDX_FRAMING_ERROR) ||
        (pending == DL_UART_IIDX_NOISE_ERROR)) {
        if (rx_instance != NULL) {
            USARTStartReceive(rx_instance);
        }
    }
}
