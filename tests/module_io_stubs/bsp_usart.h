#ifndef TEST_MODULE_IO_BSP_USART_H
#define TEST_MODULE_IO_BSP_USART_H

#include <stdint.h>

#include "bsp_def.h"

typedef struct {
    uint32_t marker;
} UART_HandleTypeDef;

typedef struct {
    uint32_t marker;
} USARTInstance;

typedef void (*usart_module_callback)(void);

typedef enum {
    USART_TRANSFER_NONE = 0,
    USART_TRANSFER_BLOCKING,
    USART_TRANSFER_IT,
    USART_TRANSFER_DMA,
} USART_TRANSFER_MODE;

typedef struct {
    uint16_t recv_buff_size;
    UART_HandleTypeDef *usart_handle;
    usart_module_callback module_callback;
} USART_Init_Config_s;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

USARTInstance *USARTRegister(USART_Init_Config_s *config);
Device_Status_e USARTSendEx(USARTInstance *instance, uint8_t *data,
    uint16_t size, USART_TRANSFER_MODE mode);
Device_Status_e USARTReceiveAvailable(USARTInstance *instance,
    uint8_t *data, uint16_t capacity, uint16_t *received_size);

#endif
