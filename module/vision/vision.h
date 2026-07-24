#ifndef MODULE_VISION_H
#define MODULE_VISION_H

#include "transparent_uart.h"

typedef struct {
    TransparentUART_Device_t transport;
} Vision_Device_t;

Device_Status_e Vision_Init(
    Vision_Device_t *device, TransparentUART_Port_e port);
Device_Status_e Vision_Send(
    Vision_Device_t *device, uint8_t *data, uint16_t size);
Device_Status_e Vision_Read(Vision_Device_t *device,
    uint8_t *data, uint16_t capacity, uint16_t *received_size);

#endif
