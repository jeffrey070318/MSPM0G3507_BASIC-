#include "vision.h"

#include <stddef.h>

Device_Status_e Vision_Init(
    Vision_Device_t *device, TransparentUART_Port_e port)
{
    if (device == NULL) {
        return DEVICE_ERROR;
    }
    return TransparentUART_Init(&device->transport, port);
}

Device_Status_e Vision_Send(
    Vision_Device_t *device, uint8_t *data, uint16_t size)
{
    if (device == NULL) {
        return DEVICE_ERROR;
    }
    return TransparentUART_Send(&device->transport, data, size);
}

Device_Status_e Vision_Read(Vision_Device_t *device,
    uint8_t *data, uint16_t capacity, uint16_t *received_size)
{
    if (device == NULL) {
        return DEVICE_ERROR;
    }
    return TransparentUART_Read(
        &device->transport, data, capacity, received_size);
}
