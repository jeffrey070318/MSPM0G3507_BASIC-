#ifndef MODULE_TRANSPARENT_UART_H
#define MODULE_TRANSPARENT_UART_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_def.h"

typedef enum {
    TRANSPARENT_UART_PORT_1 = 0,
    TRANSPARENT_UART_PORT_2,
    TRANSPARENT_UART_PORT_3,
} TransparentUART_Port_e;

typedef struct {
    TransparentUART_Port_e port;
    bool initialized;
} TransparentUART_Device_t;

Device_Status_e TransparentUART_Init(
    TransparentUART_Device_t *device, TransparentUART_Port_e port);
Device_Status_e TransparentUART_Send(
    TransparentUART_Device_t *device, uint8_t *data, uint16_t size);
Device_Status_e TransparentUART_Read(TransparentUART_Device_t *device,
    uint8_t *data, uint16_t capacity, uint16_t *received_size);

#endif
