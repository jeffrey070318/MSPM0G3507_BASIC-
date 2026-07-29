#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ball_balance.h"
#include "robot_def.h"
#include "stepper.h"
#include "vision.h"

static TransparentUART_Port_e captured_port;
static uint32_t stepper_task_count;
static uint32_t stepper_stop_count;
static uint32_t stepper_enable_count;
static uint32_t stepper_move_count;
static uint16_t captured_elapsed_ms;

Device_Status_e Vision_Init(
    Vision_Device_t *device, TransparentUART_Port_e port)
{
    assert(device != NULL);
    captured_port = port;
    return DEVICE_OK;
}

Device_Status_e Stepper_Init(Stepper_Device_t *device)
{
    assert(device != NULL);
    device->initialized = true;
    device->enabled = false;
    device->position_steps = 0;
    return DEVICE_OK;
}

Device_Status_e Stepper_Enable(Stepper_Device_t *device, bool enable)
{
    stepper_enable_count++;
    device->enabled = enable;
    return DEVICE_OK;
}

Device_Status_e Stepper_Move(Stepper_Device_t *device,
    Stepper_Direction_e direction, uint32_t steps, uint16_t speed_sps)
{
    (void) device;
    (void) direction;
    (void) steps;
    (void) speed_sps;
    stepper_move_count++;
    return DEVICE_OK;
}

void Stepper_Stop(Stepper_Device_t *device)
{
    stepper_stop_count++;
    device->running = false;
}

void Stepper_Task(Stepper_Device_t *device, uint16_t elapsed_ms)
{
    assert(device != NULL);
    stepper_task_count++;
    captured_elapsed_ms = elapsed_ms;
}

int main(void)
{
    assert(BallBalanceInit());
    assert(captured_port == BALL_BALANCE_VISION_PORT);
    assert(stepper_enable_count == 1U);

    BallBalance_Command_t command = {
        .target_position = 0.0f,
        .enabled = false,
    };
    BallBalance_Status_t status = {0};
    BallBalanceTask(&command, 1U, 0.001f, &status);
    assert(stepper_task_count == 1U);
    assert(captured_elapsed_ms == 1U);
    assert(stepper_stop_count == 1U);
    assert(!status.enabled);
    assert(!status.vision_valid);

    command.enabled = true;
    BallBalanceTask(&command, 2U, 0.001f, &status);
    assert(stepper_task_count == 2U);
    assert(stepper_move_count == 0U);
    assert(!status.enabled);
    assert(!status.vision_valid);
    assert(status.step_position == 0);
    return 0;
}
