#include "stepper.h"

#include "bsp_dwt.h"

#define STEPPER_MAX_PULSES_PER_TASK 4U

static uint16_t StepperClampSpeed(uint16_t speed_sps)
{
    if (speed_sps < STEPPER_MIN_SPEED_SPS) {
        return STEPPER_MIN_SPEED_SPS;
    }
    if (speed_sps > STEPPER_MAX_SPEED_SPS) {
        return STEPPER_MAX_SPEED_SPS;
    }
    return speed_sps;
}

static void StepperApplyEnable(const Stepper_Device_t *device, bool enable)
{
    const bool active_level =
        device->enable_active_low ? !enable : enable;
    if (active_level) {
        DL_GPIO_setPins(STEPPER_GPIO_PORT, STEPPER_GPIO_EN_PIN);
    } else {
        DL_GPIO_clearPins(STEPPER_GPIO_PORT, STEPPER_GPIO_EN_PIN);
    }
}

static void StepperApplyDirection(Stepper_Direction_e direction)
{
    if (direction == STEPPER_DIR_UP) {
        DL_GPIO_setPins(STEPPER_GPIO_PORT, STEPPER_GPIO_DIR_PIN);
    } else {
        DL_GPIO_clearPins(STEPPER_GPIO_PORT, STEPPER_GPIO_DIR_PIN);
    }
}

static void StepperPulse(const Stepper_Device_t *device)
{
    DL_GPIO_clearPins(STEPPER_GPIO_PORT, STEPPER_GPIO_STEP_PIN);
    DWT_Delay((float) device->pulse_width_us * 0.000001f);
    DL_GPIO_setPins(STEPPER_GPIO_PORT, STEPPER_GPIO_STEP_PIN);
}

Device_Status_e Stepper_Init(Stepper_Device_t *device)
{
    if (device == NULL) {
        return DEVICE_ERROR;
    }

    device->initialized = true;
    device->enabled = false;
    device->running = false;
    device->enable_active_low = true;
    device->direction = STEPPER_DIR_UP;
    device->pulse_width_us = STEPPER_DEFAULT_PULSE_WIDTH_US;
    device->speed_sps = 0U;
    device->accumulator_milli_steps = 0U;
    device->emitted_steps = 0U;
    device->remaining_steps = 0;
    device->position_steps = 0;

    DL_GPIO_setPins(STEPPER_GPIO_PORT, STEPPER_GPIO_STEP_PIN);
    DL_GPIO_clearPins(STEPPER_GPIO_PORT, STEPPER_GPIO_DIR_PIN);
    StepperApplyEnable(device, false);
    return DEVICE_OK;
}

Device_Status_e Stepper_Enable(Stepper_Device_t *device, bool enable)
{
    if ((device == NULL) || !device->initialized) {
        return DEVICE_ERROR;
    }

    device->enabled = enable;
    if (!enable) {
        device->running = false;
        device->remaining_steps = 0;
        device->accumulator_milli_steps = 0U;
    }
    StepperApplyEnable(device, enable);
    return DEVICE_OK;
}

Device_Status_e Stepper_Move(Stepper_Device_t *device,
    Stepper_Direction_e direction, uint32_t steps, uint16_t speed_sps)
{
    if ((device == NULL) || !device->initialized || (steps == 0U) ||
        (steps > (uint32_t) INT32_MAX)) {
        return DEVICE_ERROR;
    }

    device->direction = direction;
    device->speed_sps = StepperClampSpeed(speed_sps);
    device->remaining_steps = (int32_t) steps;
    device->accumulator_milli_steps = 0U;
    device->running = true;
    (void) Stepper_Enable(device, true);
    StepperApplyDirection(direction);
    return DEVICE_OK;
}

void Stepper_Stop(Stepper_Device_t *device)
{
    if (device == NULL) {
        return;
    }

    device->running = false;
    device->remaining_steps = 0;
    device->accumulator_milli_steps = 0U;
}

void Stepper_Task(Stepper_Device_t *device, uint16_t elapsed_ms)
{
    if ((device == NULL) || !device->initialized || !device->enabled ||
        !device->running || (device->speed_sps == 0U) ||
        (elapsed_ms == 0U)) {
        return;
    }

    device->accumulator_milli_steps +=
        (uint32_t) device->speed_sps * (uint32_t) elapsed_ms;

    uint8_t pulses = 0U;
    while ((device->accumulator_milli_steps >= 1000U) &&
           (pulses < STEPPER_MAX_PULSES_PER_TASK)) {
        if (device->remaining_steps <= 0) {
            Stepper_Stop(device);
            break;
        }

        device->accumulator_milli_steps -= 1000U;
        StepperPulse(device);
        device->emitted_steps++;
        device->position_steps +=
            (device->direction == STEPPER_DIR_UP) ? 1 : -1;
        device->remaining_steps--;
        pulses++;
    }
}
