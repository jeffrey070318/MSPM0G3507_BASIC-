#ifndef TEST_STEPPER_H
#define TEST_STEPPER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DEVICE_OK = 0,
    DEVICE_ERROR,
} Device_Status_e;

typedef enum {
    STEPPER_DIR_UP = 0,
    STEPPER_DIR_DOWN,
} Stepper_Direction_e;

typedef struct {
    bool initialized;
    bool enabled;
    bool running;
    int32_t position_steps;
} Stepper_Device_t;

Device_Status_e Stepper_Init(Stepper_Device_t *device);
Device_Status_e Stepper_Enable(Stepper_Device_t *device, bool enable);
Device_Status_e Stepper_Move(Stepper_Device_t *device,
    Stepper_Direction_e direction, uint32_t steps, uint16_t speed_sps);
void Stepper_Stop(Stepper_Device_t *device);
void Stepper_Task(Stepper_Device_t *device, uint16_t elapsed_ms);

#endif
