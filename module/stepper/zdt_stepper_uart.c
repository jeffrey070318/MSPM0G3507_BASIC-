#include "zdt_stepper_uart.h"

#include <stddef.h>

#define ZDT_STEPPER_CMD_SPEED 0xF6U
#define ZDT_STEPPER_FRAME_END 0x6BU

Device_Status_e ZDTStepperUART_Init(ZDTStepperUART_Device_t *device,
    TransparentUART_Port_e port, uint8_t address)
{
    if ((device == NULL) || (address == 0U)) {
        return DEVICE_ERROR;
    }

    device->initialized = false;
    if (TransparentUART_Init(&device->transport, port) != DEVICE_OK) {
        return DEVICE_ERROR;
    }

    device->address = address;
    device->initialized = true;
    return DEVICE_OK;
}

Device_Status_e ZDTStepperUART_SetSpeed(ZDTStepperUART_Device_t *device,
    int16_t signed_speed_rpm, uint8_t accel, bool sync)
{
    if ((device == NULL) || !device->initialized) {
        return DEVICE_ERROR;
    }

    uint8_t dir = (signed_speed_rpm >= 0) ? 0U : 1U;
    uint16_t speed_rpm = (signed_speed_rpm >= 0)
                             ? (uint16_t) signed_speed_rpm
                             : (uint16_t) -signed_speed_rpm;
    uint8_t frame[8] = {
        device->address,
        ZDT_STEPPER_CMD_SPEED,
        dir,
        (uint8_t) (speed_rpm >> 8),
        (uint8_t) speed_rpm,
        accel,
        sync ? 1U : 0U,
        ZDT_STEPPER_FRAME_END,
    };

    return TransparentUART_Send(&device->transport, frame, sizeof(frame));
}
