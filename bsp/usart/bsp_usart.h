#ifndef BSP_USART_H
#define BSP_USART_H

#include "bsp_device.h"

#define DEVICE_USART_CNT   1U
#define USART_RXBUFF_LIMIT 256U
#define USART_TXBUFF_LIMIT 256U
#define USART_RXQUEUE_DEPTH 2U

typedef void (*usart_module_callback)(void);

typedef enum {
    USART_TRANSFER_NONE = 0,
    USART_TRANSFER_BLOCKING,
    USART_TRANSFER_IT,
    USART_TRANSFER_DMA,
} USART_TRANSFER_MODE;

typedef struct {
    uint8_t recv_buff[USART_RXBUFF_LIMIT];
    uint8_t recv_buff_size;
    UART_HandleTypeDef *usart_handle;
    usart_module_callback module_callback;
    volatile uint16_t recv_count;
    volatile uint8_t tx_busy;
    uint8_t rx_dma_buff[USART_RXBUFF_LIMIT];
    uint8_t rx_queue[USART_RXQUEUE_DEPTH][USART_RXBUFF_LIMIT];
    uint16_t rx_queue_size[USART_RXQUEUE_DEPTH];
    volatile uint8_t rx_queue_head;
    volatile uint8_t rx_queue_tail;
    volatile uint8_t rx_queue_count;
    volatile uint32_t rx_drop_count;
    uint32_t callback_signal;
} USARTInstance;

typedef struct {
    uint8_t recv_buff_size;
    UART_HandleTypeDef *usart_handle;
    usart_module_callback module_callback;
} USART_Init_Config_s;

USARTInstance *USARTRegister(USART_Init_Config_s *init_config);
USARTInstance *USARTGetInstance(UART_HandleTypeDef *usart_handle);
void USARTServiceInit(USARTInstance *_instance);
Device_Status_e USARTSendEx(USARTInstance *_instance, uint8_t *send_buf,
    uint16_t send_size, USART_TRANSFER_MODE mode);
void USARTSend(USARTInstance *_instance, uint8_t *send_buf,
    uint16_t send_size, USART_TRANSFER_MODE mode);
uint8_t USARTIsReady(USARTInstance *_instance);

/* Called by the maintained UART vector entry in mspm0_irq.c. */
void USARTIRQHandler(void);

#endif
