#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "vision.h"

static TransparentUART_Device_t *captured_uart;
static TransparentUART_Port_e captured_port;
static uint8_t captured_data[8];
static uint16_t captured_size;
static uint8_t receive_data[8];
static uint16_t receive_size;
static Device_Status_e init_status = DEVICE_OK;

Device_Status_e TransparentUART_Init(
    TransparentUART_Device_t *device, TransparentUART_Port_e port)
{
    captured_uart = device;
    captured_port = port;
    return init_status;
}

Device_Status_e TransparentUART_Send(
    TransparentUART_Device_t *device, uint8_t *data, uint16_t size)
{
    captured_uart = device;
    if ((data == NULL) || (size > sizeof(captured_data))) {
        return DEVICE_ERROR;
    }
    memcpy(captured_data, data, size);
    captured_size = size;
    return DEVICE_OK;
}

Device_Status_e TransparentUART_Read(TransparentUART_Device_t *device,
    uint8_t *data, uint16_t capacity, uint16_t *received_size)
{
    captured_uart = device;
    if ((data == NULL) || (received_size == NULL)) {
        return DEVICE_ERROR;
    }
    uint16_t size = (receive_size < capacity) ? receive_size : capacity;
    memcpy(data, receive_data, size);
    *received_size = size;
    return DEVICE_OK;
}

int main(void)
{
    Vision_Device_t vision = {0};
    uint8_t tx[] = {0x12U, 0x34U};
    uint8_t rx[4] = {0};
    uint16_t rx_size = 0U;

    assert(Vision_Init(&vision, TRANSPARENT_UART_PORT_3) == DEVICE_OK);
    assert(captured_uart == &vision.transport);
    assert(captured_port == TRANSPARENT_UART_PORT_3);

    assert(Vision_Send(&vision, tx, sizeof(tx)) == DEVICE_OK);
    assert(captured_uart == &vision.transport);
    assert(captured_size == sizeof(tx));
    assert(memcmp(captured_data, tx, sizeof(tx)) == 0);

    receive_data[0] = 0xA5U;
    receive_data[1] = 0x5AU;
    receive_size = 2U;
    assert(Vision_Read(&vision, rx, sizeof(rx), &rx_size) == DEVICE_OK);
    assert(captured_uart == &vision.transport);
    assert(rx_size == 2U);
    assert(rx[0] == 0xA5U);
    assert(rx[1] == 0x5AU);

    assert(Vision_Init(NULL, TRANSPARENT_UART_PORT_2) == DEVICE_ERROR);
    assert(Vision_Send(NULL, tx, sizeof(tx)) == DEVICE_ERROR);
    assert(Vision_Read(NULL, rx, sizeof(rx), &rx_size) == DEVICE_ERROR);

    init_status = DEVICE_BUSY;
    assert(Vision_Init(&vision, TRANSPARENT_UART_PORT_2) == DEVICE_BUSY);
    return 0;
}
