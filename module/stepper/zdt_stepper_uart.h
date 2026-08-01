#ifndef MODULE_ZDT_STEPPER_UART_H
#define MODULE_ZDT_STEPPER_UART_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_def.h"
#include "transparent_uart.h"

typedef struct {
    TransparentUART_Device_t transport;
    uint8_t address;
    bool initialized;
} ZDTStepperUART_Device_t;

Device_Status_e ZDTStepperUART_Init(ZDTStepperUART_Device_t *device,
    TransparentUART_Port_e port, uint8_t address);
Device_Status_e ZDTStepperUART_SetSpeed(ZDTStepperUART_Device_t *device,
    int16_t signed_speed_rpm, uint8_t accel, bool sync);

#endif
