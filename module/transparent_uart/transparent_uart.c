#include "transparent_uart.h"

#include <stddef.h>

#include "bsp_usart.h"

#define TRANSPARENT_UART_PORT_COUNT 2U
#define TRANSPARENT_UART_RX_BUFFER_SIZE 16U

static USARTInstance *transparent_uart_instances[
    TRANSPARENT_UART_PORT_COUNT];

static bool TransparentUARTPortIsValid(TransparentUART_Port_e port)
{
    return (port == TRANSPARENT_UART_PORT_2) ||
           (port == TRANSPARENT_UART_PORT_3);
}

static UART_HandleTypeDef *TransparentUARTGetHandle(
    TransparentUART_Port_e port)
{
    if (port == TRANSPARENT_UART_PORT_2) {
        return &huart2;
    }
    if (port == TRANSPARENT_UART_PORT_3) {
        return &huart3;
    }
    return NULL;
}

static USARTInstance *TransparentUARTGetInstance(
    TransparentUART_Device_t *device)
{
    if ((device == NULL) || !device->initialized ||
        !TransparentUARTPortIsValid(device->port)) {
        return NULL;
    }
    return transparent_uart_instances[(uint8_t) device->port];
}

Device_Status_e TransparentUART_Init(
    TransparentUART_Device_t *device, TransparentUART_Port_e port)
{
    if ((device == NULL) || !TransparentUARTPortIsValid(port)) {
        return DEVICE_ERROR;
    }

    device->initialized = false;
    UART_HandleTypeDef *handle = TransparentUARTGetHandle(port);
    if ((handle == NULL) ||
        (transparent_uart_instances[(uint8_t) port] != NULL)) {
        return DEVICE_ERROR;
    }

    USART_Init_Config_s config = {
        .recv_buff_size = TRANSPARENT_UART_RX_BUFFER_SIZE,
        .usart_handle = handle,
        .module_callback = NULL,
    };
    USARTInstance *instance = USARTRegister(&config);
    if (instance == NULL) {
        return DEVICE_ERROR;
    }

    transparent_uart_instances[(uint8_t) port] = instance;
    device->port = port;
    device->initialized = true;
    return DEVICE_OK;
}

Device_Status_e TransparentUART_Send(
    TransparentUART_Device_t *device, uint8_t *data, uint16_t size)
{
    USARTInstance *instance = TransparentUARTGetInstance(device);
    if ((instance == NULL) || (data == NULL) || (size == 0U)) {
        return DEVICE_ERROR;
    }
    return USARTSendEx(
        instance, data, size, USART_TRANSFER_BLOCKING);
}

Device_Status_e TransparentUART_Read(TransparentUART_Device_t *device,
    uint8_t *data, uint16_t capacity, uint16_t *received_size)
{
    USARTInstance *instance = TransparentUARTGetInstance(device);
    if ((instance == NULL) || (data == NULL) || (capacity == 0U) ||
        (received_size == NULL)) {
        return DEVICE_ERROR;
    }
    return USARTReceiveAvailable(
        instance, data, capacity, received_size);
}
