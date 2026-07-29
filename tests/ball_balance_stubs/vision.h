#ifndef TEST_VISION_H
#define TEST_VISION_H

#include <stdint.h>

#include "stepper.h"

typedef enum {
    TRANSPARENT_UART_PORT_1 = 0,
    TRANSPARENT_UART_PORT_2,
    TRANSPARENT_UART_PORT_3,
} TransparentUART_Port_e;

typedef struct {
    uint32_t marker;
} Vision_Device_t;

Device_Status_e Vision_Init(
    Vision_Device_t *device, TransparentUART_Port_e port);

#endif
