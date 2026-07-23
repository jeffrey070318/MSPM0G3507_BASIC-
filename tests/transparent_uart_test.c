#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "bsp_usart.h"
#include "transparent_uart.h"

UART_HandleTypeDef huart1 = {.marker = 1U};
UART_HandleTypeDef huart2 = {.marker = 2U};
UART_HandleTypeDef huart3 = {.marker = 3U};

static USARTInstance usart_instances[2];
static USART_Init_Config_s captured_configs[2];
static uint8_t registration_count;
static bool fail_registration;
static USARTInstance *sent_instance;
static uint8_t sent_data[16];
static uint16_t sent_size;
static USART_TRANSFER_MODE sent_mode;
static uint8_t receive_data[16];
static uint16_t receive_size;

USARTInstance *USARTRegister(USART_Init_Config_s *config)
{
    if (fail_registration || (config == NULL) ||
        (registration_count >= 2U)) {
        return NULL;
    }

    captured_configs[registration_count] = *config;
    return &usart_instances[registration_count++];
}

Device_Status_e USARTSendEx(USARTInstance *instance, uint8_t *data,
    uint16_t size, USART_TRANSFER_MODE mode)
{
    if ((instance == NULL) || (data == NULL) ||
        (size == 0U) || (size > sizeof(sent_data))) {
        return DEVICE_ERROR;
    }
    sent_instance = instance;
    memcpy(sent_data, data, size);
    sent_size = size;
    sent_mode = mode;
    return DEVICE_OK;
}

Device_Status_e USARTReceiveAvailable(USARTInstance *instance,
    uint8_t *data, uint16_t capacity, uint16_t *received_size)
{
    if ((instance == NULL) || (data == NULL) ||
        (capacity == 0U) || (received_size == NULL)) {
        return DEVICE_ERROR;
    }

    uint16_t size = (receive_size < capacity) ? receive_size : capacity;
    memcpy(data, receive_data, size);
    *received_size = size;
    return DEVICE_OK;
}

int main(void)
{
    TransparentUART_Device_t uart2 = {0};
    TransparentUART_Device_t uart3 = {0};
    TransparentUART_Device_t unused = {0};
    uint8_t tx[] = {0x11U, 0x22U, 0x33U};
    uint8_t rx[4] = {0};
    uint16_t rx_size = 99U;

    assert(TransparentUART_Send(&unused, tx, sizeof(tx)) == DEVICE_ERROR);
    assert(TransparentUART_Read(
        &unused, rx, sizeof(rx), &rx_size) == DEVICE_ERROR);
    assert(TransparentUART_Init(NULL, TRANSPARENT_UART_PORT_2) ==
        DEVICE_ERROR);
    assert(TransparentUART_Init(
        &unused, (TransparentUART_Port_e) 99U) == DEVICE_ERROR);

    fail_registration = true;
    assert(TransparentUART_Init(&uart2, TRANSPARENT_UART_PORT_2) ==
        DEVICE_ERROR);
    fail_registration = false;

    assert(TransparentUART_Init(&uart2, TRANSPARENT_UART_PORT_2) ==
        DEVICE_OK);
    assert(captured_configs[0].usart_handle == &huart2);
    assert(captured_configs[0].recv_buff_size > 0U);
    assert(captured_configs[0].module_callback == NULL);
    assert(TransparentUART_Init(&unused, TRANSPARENT_UART_PORT_2) ==
        DEVICE_ERROR);

    assert(TransparentUART_Init(&uart3, TRANSPARENT_UART_PORT_3) ==
        DEVICE_OK);
    assert(captured_configs[1].usart_handle == &huart3);

    assert(TransparentUART_Send(&uart2, tx, sizeof(tx)) == DEVICE_OK);
    assert(sent_instance == &usart_instances[0]);
    assert(sent_mode == USART_TRANSFER_BLOCKING);
    assert(sent_size == sizeof(tx));
    assert(memcmp(sent_data, tx, sizeof(tx)) == 0);

    receive_data[0] = 0xA1U;
    receive_data[1] = 0xB2U;
    receive_data[2] = 0xC3U;
    receive_size = 3U;
    assert(TransparentUART_Read(
        &uart3, rx, 2U, &rx_size) == DEVICE_OK);
    assert(rx_size == 2U);
    assert(rx[0] == 0xA1U);
    assert(rx[1] == 0xB2U);

    assert(TransparentUART_Send(NULL, tx, sizeof(tx)) == DEVICE_ERROR);
    assert(TransparentUART_Send(&uart2, NULL, sizeof(tx)) == DEVICE_ERROR);
    assert(TransparentUART_Send(&uart2, tx, 0U) == DEVICE_ERROR);
    assert(TransparentUART_Read(NULL, rx, sizeof(rx), &rx_size) ==
        DEVICE_ERROR);
    assert(TransparentUART_Read(&uart3, NULL, sizeof(rx), &rx_size) ==
        DEVICE_ERROR);
    assert(TransparentUART_Read(&uart3, rx, 0U, &rx_size) == DEVICE_ERROR);
    assert(TransparentUART_Read(&uart3, rx, sizeof(rx), NULL) ==
        DEVICE_ERROR);
    return 0;
}
