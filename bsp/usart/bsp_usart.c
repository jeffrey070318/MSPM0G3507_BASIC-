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
#define USART_FIFO_DRAIN_LIMIT (16U)
#define USART_RX_ERROR_INTERRUPTS ( \
    DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR | \
    DL_UART_MAIN_INTERRUPT_BREAK_ERROR | \
    DL_UART_MAIN_INTERRUPT_PARITY_ERROR | \
    DL_UART_MAIN_INTERRUPT_FRAMING_ERROR | \
    DL_UART_MAIN_INTERRUPT_NOISE_ERROR)

#define USART_RX_REARM_INTERRUPTS ( \
    DL_UART_MAIN_INTERRUPT_DMA_DONE_RX | \
    DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR | \
    DL_UART_MAIN_INTERRUPT_RX | \
    USART_RX_ERROR_INTERRUPTS)

typedef struct {
    UART_Regs *uart;
    UART_HandleTypeDef *handle;
    uint8_t rx_dma_channel;
    uint8_t tx_dma_channel;
    IRQn_Type irqn;
    bool tx_async_supported;
} USART_HardwareContext_s;

static const USART_HardwareContext_s usart_hardware[DEVICE_USART_CNT] = {
    {UART1_INST, &huart1, DMA_CH0_CHAN_ID, DMA_CH1_CHAN_ID,
        UART1_INST_INT_IRQN, true},
    {UART2_INST, &huart2, DMA_CH2_CHAN_ID, DMA_CH3_CHAN_ID,
        UART2_INST_INT_IRQN, false},
    {UART3_INST, &huart3, DMA_CH4_CHAN_ID, DMA_CH5_CHAN_ID,
        UART3_INST_INT_IRQN, false},
};

static const USART_HardwareContext_s *USARTGetHardware(UART_Regs *uart)
{
    for (uint8_t i = 0U; i < DEVICE_USART_CNT; ++i) {
        if (usart_hardware[i].uart == uart) {
            return &usart_hardware[i];
        }
    }
    return NULL;
}

static bool USARTIsConfiguredInstance(UART_Regs *uart)
{
    return USARTGetHardware(uart) != NULL;
}

static bool USARTSupportsAsyncTransmit(UART_Regs *uart)
{
    const USART_HardwareContext_s *hardware = USARTGetHardware(uart);
    return (hardware != NULL) && hardware->tx_async_supported;
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
        (instance->recv_buff_size == 0U) ||
        (instance->recv_buff_size > USART_RXBUFF_LIMIT)) {
        return;
    }

    const USART_HardwareContext_s *hardware =
        USARTGetHardware(instance->usart_handle->Instance);
    if (hardware == NULL) {
        return;
    }

    DL_UART_Main_disableDMAReceiveEvent(
        hardware->uart, DL_UART_DMA_INTERRUPT_RX);
    DL_DMA_disableChannel(DMA, hardware->rx_dma_channel);
    DL_UART_Main_clearInterruptStatus(
        hardware->uart, USART_RX_REARM_INTERRUPTS);
    DL_DMA_setSrcAddr(DMA, hardware->rx_dma_channel,
        (uint32_t) &hardware->uart->RXDATA);
    DL_DMA_setDestAddr(DMA, hardware->rx_dma_channel,
        (uint32_t) instance->rx_dma_buff);
    DL_DMA_setTransferSize(DMA, hardware->rx_dma_channel,
        instance->recv_buff_size);
    DL_DMA_enableChannel(DMA, hardware->rx_dma_channel);
    DL_UART_Main_enableDMAReceiveEvent(
        hardware->uart, DL_UART_DMA_INTERRUPT_RX);

    NVIC_ClearPendingIRQ(hardware->irqn);
#ifdef USE_FREERTOS
    NVIC_SetPriority(hardware->irqn,
        configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
#endif
    NVIC_EnableIRQ(hardware->irqn);
}

static void USARTFinishAsyncTransmit(void)
{
    USARTInstance *instance = active_tx_instance;
    UART_Regs *uart = (instance != NULL) &&
            (instance->usart_handle != NULL)
        ? instance->usart_handle->Instance
        : UART1_INST;
    DL_UART_Main_disableInterrupt(uart,
        DL_UART_MAIN_INTERRUPT_TX |
            DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
            DL_UART_MAIN_INTERRUPT_EOT_DONE);
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

static void USARTCommitReceive(
    USARTInstance *instance, const uint8_t *data, uint16_t size)
{
    if ((instance == NULL) || (data == NULL) || (size == 0U)) {
        return;
    }

    if (instance->module_callback == NULL) {
        (void) USARTRxRingWrite(&instance->rx_ring, data, size);
        instance->recv_count = USARTRxRingCount(&instance->rx_ring);
        instance->rx_drop_count = instance->rx_ring.drop_count;
        return;
    }

    uint16_t offset = 0U;
    while (offset < size) {
        uint16_t chunk = (uint16_t) (size - offset);
        if (chunk > instance->recv_buff_size) {
            chunk = instance->recv_buff_size;
        }

#ifdef USE_FREERTOS
        if (instance->rx_queue_count < USART_RXQUEUE_DEPTH) {
            uint8_t head = instance->rx_queue_head;
            memcpy(instance->rx_queue[head], &data[offset], chunk);
            instance->rx_queue_size[head] = chunk;
            instance->rx_queue_head =
                (uint8_t) ((head + 1U) % USART_RXQUEUE_DEPTH);
            instance->rx_queue_count++;
            (void) NotifyCallbackTaskFromISR(instance->callback_signal);
        } else {
            instance->rx_drop_count += chunk;
        }
#else
        memcpy(instance->recv_buff, &data[offset], chunk);
        instance->recv_count = chunk;
        instance->module_callback();
#endif
        offset = (uint16_t) (offset + chunk);
    }
}

static void USARTCollectReceive(USARTInstance *instance)
{
    if ((instance == NULL) || (instance->usart_handle == NULL)) {
        return;
    }

    const USART_HardwareContext_s *hardware =
        USARTGetHardware(instance->usart_handle->Instance);
    if (hardware == NULL) {
        return;
    }

    DL_UART_Main_disableDMAReceiveEvent(
        hardware->uart, DL_UART_DMA_INTERRUPT_RX);
    DL_DMA_disableChannel(DMA, hardware->rx_dma_channel);

    uint16_t remaining =
        DL_DMA_getTransferSize(DMA, hardware->rx_dma_channel);
    uint16_t received = (remaining <= instance->recv_buff_size)
        ? (uint16_t) (instance->recv_buff_size - remaining)
        : 0U;
    USARTCommitReceive(instance, instance->rx_dma_buff, received);

    uint8_t fifo_data[USART_FIFO_DRAIN_LIMIT];
    uint16_t fifo_count = 0U;
    while (!DL_UART_Main_isRXFIFOEmpty(hardware->uart)) {
        fifo_data[fifo_count++] =
            (uint8_t) DL_UART_Main_receiveData(hardware->uart);
        if (fifo_count == USART_FIFO_DRAIN_LIMIT) {
            USARTCommitReceive(instance, fifo_data, fifo_count);
            fifo_count = 0U;
        }
    }
    USARTCommitReceive(instance, fifo_data, fifo_count);

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
        !USARTSupportsAsyncTransmit(instance->usart_handle->Instance) ||
        (data == NULL) || (size == 0U) ||
        (size > USART_TXBUFF_LIMIT)) {
        return DEVICE_ERROR;
    }

    const USART_HardwareContext_s *hardware =
        USARTGetHardware(instance->usart_handle->Instance);
    if (hardware == NULL) {
        return DEVICE_ERROR;
    }

    uint32_t critical_state = USARTEnterCritical();
    if ((active_tx_instance != NULL) || (instance->tx_busy != 0U)) {
        USARTExitCritical(critical_state);
        return DEVICE_BUSY;
    }

    DL_UART_Main_disableInterrupt(hardware->uart,
        DL_UART_MAIN_INTERRUPT_TX |
            DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
            DL_UART_MAIN_INTERRUPT_EOT_DONE);
    DL_UART_Main_clearInterruptStatus(hardware->uart,
        DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
            DL_UART_MAIN_INTERRUPT_EOT_DONE);
    NVIC_ClearPendingIRQ(hardware->irqn);
    active_tx_instance = instance;
    instance->tx_busy = 1U;
    USARTTxCompletionBegin(&tx_completion);
    USARTExitCritical(critical_state);

    memcpy(tx_buffer, data, size);
    tx_size = size;
    tx_index = 0U;

    NVIC_EnableIRQ(hardware->irqn);

    if (use_dma) {
        DL_DMA_disableChannel(DMA, hardware->tx_dma_channel);
        DL_DMA_setSrcAddr(
            DMA, hardware->tx_dma_channel, (uint32_t) tx_buffer);
        DL_DMA_setDestAddr(DMA, hardware->tx_dma_channel,
            (uint32_t) &hardware->uart->TXDATA);
        DL_DMA_setTransferSize(
            DMA, hardware->tx_dma_channel, size);
        DL_UART_Main_clearInterruptStatus(hardware->uart,
            DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
                DL_UART_MAIN_INTERRUPT_EOT_DONE);
        DL_UART_Main_enableInterrupt(hardware->uart,
            DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
                DL_UART_MAIN_INTERRUPT_EOT_DONE);
        DL_DMA_enableChannel(DMA, hardware->tx_dma_channel);
    } else {
        DL_UART_Main_clearInterruptStatus(
            hardware->uart, DL_UART_MAIN_INTERRUPT_EOT_DONE);
        DL_UART_Main_enableInterrupt(hardware->uart,
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
    USARTRxRingInit(
        &instance->rx_ring, instance->recv_buff, USART_RXBUFF_LIMIT);
    if (USARTSupportsAsyncTransmit(instance->usart_handle->Instance)) {
        DL_UART_Main_disableInterrupt(instance->usart_handle->Instance,
            DL_UART_MAIN_INTERRUPT_TX |
                DL_UART_MAIN_INTERRUPT_DMA_DONE_TX |
                DL_UART_MAIN_INTERRUPT_EOT_DONE);
        DL_UART_Main_clearInterruptStatus(instance->usart_handle->Instance,
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
            (usart_instance[i]->usart_handle != NULL) &&
            (usart_instance[i]->usart_handle->Instance ==
                usart_handle->Instance)) {
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

    uint32_t critical_state = USARTEnterCritical();
    *received_size =
        USARTRxRingRead(&_instance->rx_ring, data, capacity);
    _instance->recv_count = USARTRxRingCount(&_instance->rx_ring);
    _instance->rx_drop_count = _instance->rx_ring.drop_count;
    USARTExitCritical(critical_state);
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

void USARTIRQHandlerFor(UART_Regs *uart)
{
    const USART_HardwareContext_s *hardware = USARTGetHardware(uart);
    if (hardware == NULL) {
        return;
    }

    USARTInstance *instance = USARTGetInstance(hardware->handle);
    if (instance == NULL) {
        /* A stale/premature IRQ must not livelock the CPU before registration. */
        DL_UART_Main_disableInterrupt(uart, USART_RX_REARM_INTERRUPTS);
        DL_UART_Main_clearInterruptStatus(uart, USART_RX_REARM_INTERRUPTS);
        return;
    }

    for (;;) {
        DL_UART_IIDX pending = DL_UART_Main_getPendingInterrupt(uart);
        if (pending == DL_UART_IIDX_NO_INTERRUPT) {
            return;
        }

        switch (pending) {
        case DL_UART_IIDX_DMA_DONE_RX:
        case DL_UART_IIDX_RX_TIMEOUT_ERROR:
        case DL_UART_IIDX_RX:
            USARTCollectReceive(instance);
            break;

        case DL_UART_IIDX_DMA_DONE_TX:
            if (!hardware->tx_async_supported) {
                DL_UART_Main_disableInterrupt(
                    uart, DL_UART_MAIN_INTERRUPT_DMA_DONE_TX);
                break;
            }
            DL_DMA_disableChannel(DMA, hardware->tx_dma_channel);
            if (!tx_completion.active) {
                DL_UART_Main_disableInterrupt(
                    uart, DL_UART_MAIN_INTERRUPT_DMA_DONE_TX);
                break;
            }
            if (USARTTxCompletionOnDataLoaded(&tx_completion) &&
                !DL_UART_Main_isBusy(uart)) {
                USARTFinishAsyncTransmit();
            }
            break;

        case DL_UART_IIDX_TX:
            if (!hardware->tx_async_supported) {
                DL_UART_Main_disableInterrupt(
                    uart, DL_UART_MAIN_INTERRUPT_TX);
                break;
            }
            while ((tx_index < tx_size) &&
                   !DL_UART_Main_isTXFIFOFull(uart)) {
                DL_UART_Main_transmitData(uart, tx_buffer[tx_index++]);
            }
            if (tx_index >= tx_size) {
                DL_UART_Main_disableInterrupt(
                    uart, DL_UART_MAIN_INTERRUPT_TX);
                if (USARTTxCompletionOnDataLoaded(&tx_completion) &&
                    !DL_UART_Main_isBusy(uart)) {
                    USARTFinishAsyncTransmit();
                }
            }
            break;

        case DL_UART_IIDX_EOT_DONE:
            if (!hardware->tx_async_supported) {
                DL_UART_Main_disableInterrupt(
                    uart, DL_UART_MAIN_INTERRUPT_EOT_DONE);
                break;
            }
            if (!tx_completion.active) {
                DL_UART_Main_disableInterrupt(
                    uart, DL_UART_MAIN_INTERRUPT_EOT_DONE);
                break;
            }
            if (USARTTxCompletionOnEOT(&tx_completion)) {
                USARTFinishAsyncTransmit();
            }
            break;

        case DL_UART_IIDX_OVERRUN_ERROR:
        case DL_UART_IIDX_BREAK_ERROR:
        case DL_UART_IIDX_PARITY_ERROR:
        case DL_UART_IIDX_FRAMING_ERROR:
        case DL_UART_IIDX_NOISE_ERROR:
            instance->rx_error_count++;
            if (pending == DL_UART_IIDX_OVERRUN_ERROR) {
                instance->rx_overrun_count++;
            }
            USARTCollectReceive(instance);
            break;

        default:
            return;
        }
    }
}

void USARTIRQHandler(void)
{
    USARTIRQHandlerFor(UART1_INST);
}
