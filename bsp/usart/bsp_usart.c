#include "bsp_usart.h"
#include "bsp_memory.h"
#include "bsp_tools.h"
#include "bsp_usart_state.h"

#include <string.h>

#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

static uint8_t idx;
static USARTInstance *usart_instance[DEVICE_USART_CNT] = {NULL};

static USARTInstance * volatile active_tx_instance;
static uint8_t tx_buffer[USART_TXBUFF_LIMIT];
static volatile uint16_t tx_size;
static volatile uint16_t tx_index;
static USART_Tx_Completion_State_s tx_completion;

#define USART_POLL_LIMIT (1000000U)

static bool USARTIsConfiguredInstance(UART_Regs *uart)
{
    return (uart == UART1_INST) || (uart == UART2_INST) ||
           (uart == UART3_INST);
}

static bool USARTSupportsAsync(UART_Regs *uart)
{
    return uart == UART1_INST;
}

static uint32_t USARTEnterCritical(void)
{
#ifdef USE_FREERTOS
    taskENTER_CRITICAL();
    return 0U;
#else
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
#endif
}

static void USARTExitCritical(uint32_t state)
{
#ifdef USE_FREERTOS
    (void) state;
    taskEXIT_CRITICAL();
#else
    if (state == 0U) {
        __enable_irq();
    }
#endif
}

static void USARTStartReceive(USARTInstance *instance)
{
    if ((instance == NULL) || (instance->usart_handle == NULL) ||
        !USARTSupportsAsync(instance->usart_handle->Instance) ||
        (instance->recv_buff_size == 0U) ||
        (instance->recv_buff_size > USART_RXBUFF_LIMIT)) {
        return;
    }

    instance->recv_count = 0U;
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    DL_UART_Main_clearInterruptStatus(instance->usart_handle->Instance,
        DL_UART_MAIN_INTERRUPT_DMA_DONE_RX |
            DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t) &instance->usart_handle->Instance->RXDATA);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t) instance->rx_dma_buff);
    DL_DMA_setTransferSize(
        DMA, DMA_CH0_CHAN_ID, instance->recv_buff_size);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    NVIC_ClearPendingIRQ(UART1_INST_INT_IRQN);
#ifdef USE_FREERTOS
    NVIC_SetPriority(UART1_INST_INT_IRQN,
        configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
#endif
    NVIC_EnableIRQ(UART1_INST_INT_IRQN);
}

static void USARTFinishAsyncTransmit(void)
{
    DL_UART_Main_disableInterrupt(UART1_INST,
        DL_UART_MAIN_INTERRUPT_TX |
            DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
            DL_UART_MAIN_INTERRUPT_EOT_DONE);
    USARTInstance *instance = active_tx_instance;
    if (instance != NULL) {
        instance->tx_busy = 0U;
    }
    active_tx_instance = NULL;
    tx_size = 0U;
    tx_index = 0U;
    USARTTxCompletionReset(&tx_completion);
}

#ifdef USE_FREERTOS
static void USARTRunReceiveCallback(void const *argument)
{
    USARTInstance *instance = (USARTInstance *) argument;
    if (instance == NULL) {
        return;
    }

    for (;;) {
        uint16_t size;
        uint32_t critical_state = USARTEnterCritical();
        if (instance->rx_queue_count == 0U) {
            USARTExitCritical(critical_state);
            return;
        }

        uint8_t tail = instance->rx_queue_tail;
        size = instance->rx_queue_size[tail];
        memcpy(instance->recv_buff, instance->rx_queue[tail], size);
        instance->recv_count = size;
        instance->rx_queue_tail =
            (uint8_t) ((tail + 1U) % USART_RXQUEUE_DEPTH);
        instance->rx_queue_count--;
        USARTExitCritical(critical_state);

        if (instance->module_callback != NULL) {
            instance->module_callback();
        }
    }
}
#endif

static void USARTDispatchReceive(USARTInstance *instance, uint16_t size)
{
    if ((instance == NULL) || (size == 0U) ||
        (size > instance->recv_buff_size)) {
        return;
    }

#ifdef USE_FREERTOS
    if (instance->module_callback == NULL) {
        memcpy(instance->recv_buff, instance->rx_dma_buff, size);
        instance->recv_count = size;
    } else if (instance->rx_queue_count < USART_RXQUEUE_DEPTH) {
        uint8_t head = instance->rx_queue_head;
        memcpy(instance->rx_queue[head], instance->rx_dma_buff, size);
        instance->rx_queue_size[head] = size;
        instance->rx_queue_head =
            (uint8_t) ((head + 1U) % USART_RXQUEUE_DEPTH);
        instance->rx_queue_count++;
        (void) NotifyCallbackTaskFromISR(instance->callback_signal);
    } else {
        instance->rx_drop_count++;
    }
#else
    memcpy(instance->recv_buff, instance->rx_dma_buff, size);
    instance->recv_count = size;
    if (instance->module_callback != NULL) {
        instance->module_callback();
    }
#endif

    memset(instance->rx_dma_buff, 0, size);
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

    uint32_t critical_state = USARTEnterCritical();
    if (instance->tx_busy != 0U) {
        USARTExitCritical(critical_state);
        return DEVICE_BUSY;
    }
    instance->tx_busy = 1U;
    USARTExitCritical(critical_state);

    for (uint16_t i = 0U; i < size; ++i) {
        uint32_t poll = 0U;
        while (DL_UART_Main_isTXFIFOFull(instance->usart_handle->Instance)) {
            if (++poll >= USART_POLL_LIMIT) {
                critical_state = USARTEnterCritical();
                instance->tx_busy = 0U;
                USARTExitCritical(critical_state);
                return DEVICE_TIMEOUT;
            }
        }
        DL_UART_Main_transmitData(instance->usart_handle->Instance, data[i]);
    }
    uint32_t poll = 0U;
    while (DL_UART_Main_isBusy(instance->usart_handle->Instance)) {
        if (++poll >= USART_POLL_LIMIT) {
            critical_state = USARTEnterCritical();
            instance->tx_busy = 0U;
            USARTExitCritical(critical_state);
            return DEVICE_TIMEOUT;
        }
    }
    critical_state = USARTEnterCritical();
    instance->tx_busy = 0U;
    USARTExitCritical(critical_state);
    return DEVICE_OK;
}

static Device_Status_e USARTSendAsync(USARTInstance *instance,
    uint8_t *data, uint16_t size, bool use_dma)
{
    if ((instance == NULL) || (instance->usart_handle == NULL) ||
        !USARTSupportsAsync(instance->usart_handle->Instance) ||
        (data == NULL) || (size == 0U) ||
        (size > USART_TXBUFF_LIMIT)) {
        return DEVICE_ERROR;
    }
    uint32_t critical_state = USARTEnterCritical();
    if ((active_tx_instance != NULL) || (instance->tx_busy != 0U)) {
        USARTExitCritical(critical_state);
        return DEVICE_BUSY;
    }

    DL_UART_Main_disableInterrupt(UART1_INST,
        DL_UART_MAIN_INTERRUPT_TX |
            DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
            DL_UART_MAIN_INTERRUPT_EOT_DONE);
    DL_UART_Main_clearInterruptStatus(UART1_INST,
        DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
            DL_UART_MAIN_INTERRUPT_EOT_DONE);
    NVIC_ClearPendingIRQ(UART1_INST_INT_IRQN);
    active_tx_instance = instance;
    instance->tx_busy = 1U;
    USARTTxCompletionBegin(&tx_completion);
    USARTExitCritical(critical_state);

    memcpy(tx_buffer, data, size);
    tx_size = size;
    tx_index = 0U;

    NVIC_EnableIRQ(UART1_INST_INT_IRQN);

    if (use_dma) {
        DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
        DL_DMA_setSrcAddr(
            DMA, DMA_CH1_CHAN_ID, (uint32_t) tx_buffer);
        DL_DMA_setDestAddr(DMA, DMA_CH1_CHAN_ID,
            (uint32_t) &UART1_INST->TXDATA);
        DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, size);
        DL_UART_Main_clearInterruptStatus(UART1_INST,
            DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
                DL_UART_MAIN_INTERRUPT_EOT_DONE);
        DL_UART_Main_enableInterrupt(UART1_INST,
            DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
                DL_UART_MAIN_INTERRUPT_EOT_DONE);
        DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
    } else {
        DL_UART_Main_clearInterruptStatus(
            UART1_INST, DL_UART_MAIN_INTERRUPT_EOT_DONE);
        DL_UART_Main_enableInterrupt(UART1_INST,
            DL_UART_MAIN_INTERRUPT_TX |
                DL_UART_MAIN_INTERRUPT_EOT_DONE);
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
        !USARTIsConfiguredInstance(init_config->usart_handle->Instance) ||
        ((init_config->usart_handle->Instance != UART1_INST) &&
            (init_config->module_callback != NULL)) ||
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
        (USARTInstance *) BSPMalloc(sizeof(USARTInstance));
    if (instance == NULL) {
        return NULL;
    }
    memset(instance, 0, sizeof(USARTInstance));

    instance->usart_handle = init_config->usart_handle;
    instance->recv_buff_size = init_config->recv_buff_size;
    instance->module_callback = init_config->module_callback;
    if (USARTSupportsAsync(instance->usart_handle->Instance)) {
        DL_UART_Main_disableInterrupt(UART1_INST,
            DL_UART_MAIN_INTERRUPT_TX |
                DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
                DL_UART_MAIN_INTERRUPT_EOT_DONE);
        DL_UART_Main_clearInterruptStatus(UART1_INST,
            DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
                DL_UART_MAIN_INTERRUPT_EOT_DONE);
    }
#ifdef USE_FREERTOS
    if (instance->module_callback != NULL) {
        instance->callback_signal = CreateCallbackTask("uart_rx",
            USARTRunReceiveCallback, instance, tskIDLE_PRIORITY + 4U);
        if (instance->callback_signal == 0U) {
            BSPFree(instance);
            return NULL;
        }
    }
#endif
    usart_instance[idx++] = instance;

    USARTServiceInit(instance);
    return instance;
}

USARTInstance *USARTGetInstance(UART_HandleTypeDef *usart_handle)
{
    if (usart_handle == NULL) {
        return NULL;
    }

    for (uint8_t i = 0U; i < idx; ++i) {
        if ((usart_instance[i] != NULL) &&
            (usart_instance[i]->usart_handle == usart_handle)) {
            return usart_instance[i];
        }
    }
    return NULL;
}

Device_Status_e USARTSendEx(USARTInstance *_instance, uint8_t *send_buf,
    uint16_t send_size, USART_TRANSFER_MODE mode)
{
    switch (mode) {
    case USART_TRANSFER_BLOCKING:
        return USARTSendBlocking(_instance, send_buf, send_size);
    case USART_TRANSFER_IT:
        return USARTSendAsync(_instance, send_buf, send_size, false);
    case USART_TRANSFER_DMA:
        return USARTSendAsync(_instance, send_buf, send_size, true);
    default:
        return DEVICE_ERROR;
    }
}

Device_Status_e USARTReceiveAvailable(USARTInstance *_instance,
    uint8_t *data, uint16_t capacity, uint16_t *received_size)
{
    if ((_instance == NULL) || (_instance->usart_handle == NULL) ||
        (_instance->usart_handle->Instance == NULL) || (data == NULL) ||
        (capacity == 0U) || (received_size == NULL)) {
        return DEVICE_ERROR;
    }

    *received_size = 0U;
    while ((*received_size < capacity) &&
           !DL_UART_Main_isRXFIFOEmpty(_instance->usart_handle->Instance)) {
        data[*received_size] =
            DL_UART_Main_receiveData(_instance->usart_handle->Instance);
        (*received_size)++;
    }
    return DEVICE_OK;
}

void USARTSend(USARTInstance *_instance, uint8_t *send_buf,
    uint16_t send_size, USART_TRANSFER_MODE mode)
{
    (void) USARTSendEx(_instance, send_buf, send_size, mode);
}

uint8_t USARTIsReady(USARTInstance *_instance)
{
    if (_instance == NULL) {
        return 0U;
    }
    uint32_t critical_state = USARTEnterCritical();
    uint8_t ready = (_instance->tx_busy == 0U) ? 1U : 0U;
    USARTExitCritical(critical_state);
    return ready;
}

void USARTIRQHandler(void)
{
    DL_UART_IIDX pending = DL_UART_Main_getPendingInterrupt(UART1_INST);
    USARTInstance *rx_instance = USARTGetInstance(&huart1);

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
        if (!tx_completion.active) {
            DL_UART_Main_disableInterrupt(
                UART1_INST, DL_UART_MAIN_INTERRUPT_DMA_DONE_TX);
            return;
        }
        if (USARTTxCompletionOnDataLoaded(&tx_completion) &&
            !DL_UART_Main_isBusy(UART1_INST)) {
            USARTFinishAsyncTransmit();
        }
        return;
    }

    if (pending == DL_UART_IIDX_TX) {
        while ((tx_index < tx_size) &&
               !DL_UART_Main_isTXFIFOFull(UART1_INST)) {
            DL_UART_Main_transmitData(
                UART1_INST, tx_buffer[tx_index++]);
        }
        if (tx_index >= tx_size) {
            DL_UART_Main_disableInterrupt(
                UART1_INST, DL_UART_MAIN_INTERRUPT_TX);
            if (USARTTxCompletionOnDataLoaded(&tx_completion) &&
                !DL_UART_Main_isBusy(UART1_INST)) {
                USARTFinishAsyncTransmit();
            }
        }
        return;
    }

    if (pending == DL_UART_IIDX_EOT_DONE) {
        if (!tx_completion.active) {
            DL_UART_Main_disableInterrupt(
                UART1_INST, DL_UART_MAIN_INTERRUPT_EOT_DONE);
            return;
        }
        if (USARTTxCompletionOnEOT(&tx_completion)) {
            USARTFinishAsyncTransmit();
        }
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
