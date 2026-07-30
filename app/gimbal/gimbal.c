#include "gimbal.h"

#include "stepper.h"

#define GIMBAL_TASK_PERIOD_MS          5U
#define GIMBAL_STEPPER_SWING_STEPS    40U
#define GIMBAL_STEPPER_SWING_SPEED_SPS 200U

static Stepper_Device_t gimbal_stepper;
static Stepper_Direction_e gimbal_stepper_next_direction = STEPPER_DIR_UP;

volatile Device_Status_e gimbal_stepper_init_status = DEVICE_ERROR;
volatile uint32_t gimbal_stepper_task_count;
volatile int32_t gimbal_stepper_position_steps;
volatile bool gimbal_stepper_running;

void GimbalInit(void)
{
    gimbal_stepper_init_status = Stepper_Init(&gimbal_stepper);
}

void GimbalTask(void)
{
    if (gimbal_stepper_init_status != DEVICE_OK) {
        return;
    }

    if (!gimbal_stepper.running) {
        (void) Stepper_Move(&gimbal_stepper, gimbal_stepper_next_direction,
            GIMBAL_STEPPER_SWING_STEPS, GIMBAL_STEPPER_SWING_SPEED_SPS);
        gimbal_stepper_next_direction =
            (gimbal_stepper_next_direction == STEPPER_DIR_UP)
                ? STEPPER_DIR_DOWN
                : STEPPER_DIR_UP;
    }

    Stepper_Task(&gimbal_stepper, GIMBAL_TASK_PERIOD_MS);

    gimbal_stepper_task_count++;
    gimbal_stepper_position_steps = gimbal_stepper.position_steps;
    gimbal_stepper_running = gimbal_stepper.running;
}
