#include "gimbal.h"

#include "key.h"
#include "stepper.h"
#include "ti_msp_dl_config.h"

#define GIMBAL_TASK_PERIOD_MS          5U
#define GIMBAL_STEPPER_SWING_STEPS    40U
#define GIMBAL_STEPPER_SWING_SPEED_SPS 200U
#define GIMBAL_DEFAULT_PIPE_ROUND_TRIPS 5U

static Stepper_Device_t gimbal_stepper;
static Stepper_Direction_e gimbal_stepper_next_direction = STEPPER_DIR_UP;
static KEY_Device_t gimbal_key2;
static bool gimbal_key2_was_pressed;
static uint8_t pending_pipe_round_trips;

volatile Device_Status_e gimbal_stepper_init_status = DEVICE_ERROR;
volatile uint32_t gimbal_stepper_task_count;
volatile int32_t gimbal_stepper_position_steps;
volatile bool gimbal_stepper_running;
volatile Device_Status_e gimbal_key2_init_status = DEVICE_ERROR;
volatile uint32_t gimbal_key2_press_count;
volatile uint32_t gimbal_key_active_level;
volatile bool gimbal_vision_pipe_request_pending;
volatile uint8_t gimbal_pipe_round_trip_target =
    GIMBAL_DEFAULT_PIPE_ROUND_TRIPS;

static GPIO_PinState GimbalKeyActiveState(void)
{
    return (gimbal_key_active_level != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

static bool GimbalKey2Pressed(void)
{
    gimbal_key2.active_state = GimbalKeyActiveState();
    return KEY_IsPressed(&gimbal_key2);
}

static void GimbalRequestVisionPipe(void)
{
    pending_pipe_round_trips = gimbal_pipe_round_trip_target;
    if (pending_pipe_round_trips == 0U) {
        pending_pipe_round_trips = GIMBAL_DEFAULT_PIPE_ROUND_TRIPS;
    }
    gimbal_vision_pipe_request_pending = true;
}

void GimbalInit(void)
{
    gimbal_stepper_init_status = Stepper_Init(&gimbal_stepper);
    gimbal_key2_init_status =
        KEY_Init(&gimbal_key2, KEY_GPIO_KEY2_PORT, KEY_GPIO_KEY2_PIN,
            GimbalKeyActiveState())
            ? DEVICE_OK
            : DEVICE_ERROR;
    if (gimbal_key2_init_status == DEVICE_OK) {
        gimbal_key2_was_pressed = GimbalKey2Pressed();
    }
}

void GimbalTask(void)
{
    if (gimbal_key2_init_status == DEVICE_OK) {
        bool key2_pressed = GimbalKey2Pressed();
        if (key2_pressed && !gimbal_key2_was_pressed) {
            gimbal_key2_press_count++;
            GimbalRequestVisionPipe();
        }
        gimbal_key2_was_pressed = key2_pressed;
    }

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

bool GimbalTakeVisionPipeRequest(uint8_t *round_trips)
{
    if (!gimbal_vision_pipe_request_pending) {
        return false;
    }

    if (round_trips != NULL) {
        *round_trips = pending_pipe_round_trips;
    }
    gimbal_vision_pipe_request_pending = false;
    return true;
}
