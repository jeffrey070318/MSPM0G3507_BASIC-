#include "ball_balance.h"

#include <stddef.h>

#include "robot_def.h"
#include "stepper.h"
#include "vision.h"

static Vision_Device_t g_vision;
static Stepper_Device_t g_stepper;
static bool g_initialized;

static void BallBalanceWriteStatus(BallBalance_Status_t *status)
{
    if (status == NULL) {
        return;
    }

    status->measured_position = 0.0f;
    status->step_position = g_stepper.position_steps;
    status->vision_valid = false;
    status->enabled = false;
    status->at_soft_limit = false;
}

static void BallBalanceSafeStop(void)
{
    Stepper_Stop(&g_stepper);
    (void) Stepper_Enable(&g_stepper, false);
}

bool BallBalanceInit(void)
{
    g_initialized = false;
    if (Vision_Init(&g_vision, BALL_BALANCE_VISION_PORT) != DEVICE_OK) {
        return false;
    }
    if (Stepper_Init(&g_stepper) != DEVICE_OK) {
        return false;
    }

    (void) Stepper_Enable(&g_stepper, false);
    g_initialized = true;
    return true;
}

void BallBalanceTask(const BallBalance_Command_t *command,
    uint32_t now_ms, float dt_seconds, BallBalance_Status_t *status)
{
    (void) now_ms;
    (void) dt_seconds;

    if (!g_initialized) {
        BallBalanceWriteStatus(status);
        return;
    }

    Stepper_Task(&g_stepper, 1U);

    /* Camera framing is not frozen yet. No byte stream is treated as valid. */
    if ((command == NULL) || !command->enabled) {
        BallBalanceSafeStop();
        BallBalanceWriteStatus(status);
        return;
    }

    BallBalanceSafeStop();
    BallBalanceWriteStatus(status);
}
