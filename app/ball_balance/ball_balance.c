#include "ball_balance.h"

#include <stddef.h>

#include "robot_def.h"
#include "stepper.h"
#include "vision.h"
#include "zdt_stepper_uart.h"

typedef enum {
    BALL_BALANCE_OPEN_IDLE = 0,
    BALL_BALANCE_OPEN_POS_GO,
    BALL_BALANCE_OPEN_NEG_GO,
    BALL_BALANCE_OPEN_FINAL_BUMP_POS,
    BALL_BALANCE_OPEN_FINAL_BUMP_NEG,
    BALL_BALANCE_OPEN_FINAL_HOLD,
} BallBalance_Open_State_t;

static Vision_Device_t g_vision;
static Stepper_Device_t g_stepper;
static ZDTStepperUART_Device_t g_stepper_uart;
static bool g_level_confirmed;
static bool g_command_was_enabled;
static bool g_open_active;
static bool g_open_state_commanded;
static BallBalance_Open_State_t g_open_state;
static uint32_t g_open_state_start_ms;
static bool g_uart_speed_active;
static int16_t g_last_uart_speed_rpm;
static uint32_t g_last_uart_command_ms;
static bool g_initialized;

volatile int32_t ball_balance_open_pos_go_target_steps =
    BALL_BALANCE_OPEN_POS_GO_TARGET_STEPS;
volatile uint32_t ball_balance_open_pos_go_hold_ms =
    BALL_BALANCE_OPEN_POS_GO_HOLD_MS;
volatile int32_t ball_balance_open_neg_go_target_steps =
    BALL_BALANCE_OPEN_NEG_GO_TARGET_STEPS;
volatile uint32_t ball_balance_open_neg_go_hold_ms =
    BALL_BALANCE_OPEN_NEG_GO_HOLD_MS;
volatile int32_t ball_balance_open_final_bump_pos_target_steps =
    BALL_BALANCE_OPEN_FINAL_BUMP_POS_TARGET_STEPS;
volatile uint32_t ball_balance_open_final_bump_pos_hold_ms =
    BALL_BALANCE_OPEN_FINAL_BUMP_POS_HOLD_MS;
volatile int32_t ball_balance_open_final_bump_neg_target_steps =
    BALL_BALANCE_OPEN_FINAL_BUMP_NEG_TARGET_STEPS;
volatile uint32_t ball_balance_open_final_bump_neg_hold_ms =
    BALL_BALANCE_OPEN_FINAL_BUMP_NEG_HOLD_MS;
volatile int32_t ball_balance_open_final_hold_target_steps =
    BALL_BALANCE_OPEN_FINAL_HOLD_TARGET_STEPS;
volatile uint16_t ball_balance_open_speed_sps =
    BALL_BALANCE_OPEN_LOOP_SPEED_SPS;

volatile bool ball_balance_debug_level_key_pressed;
volatile bool ball_balance_debug_level_confirmed;
volatile bool ball_balance_debug_open_active;
volatile uint8_t ball_balance_debug_open_state;
volatile bool ball_balance_debug_open_state_commanded;
volatile int32_t ball_balance_debug_step_position;
volatile int32_t ball_balance_debug_last_target_steps;
volatile int32_t ball_balance_debug_rejected_target_steps;
volatile uint32_t ball_balance_debug_move_error_count;
volatile int16_t ball_balance_debug_uart_speed_rpm;
volatile uint32_t ball_balance_debug_uart_command_count;
volatile Device_Status_e ball_balance_debug_uart_last_status;

#if BALL_BALANCE_SOFT_LIMIT_STEPS > 0
static int32_t BallBalanceAbsSteps(int32_t steps)
{
    return (steps < 0) ? -steps : steps;
}
#endif

static void BallBalanceWriteStatus(
    BallBalance_Status_t *status, bool enabled)
{
    if (status == NULL) {
        return;
    }

    status->measured_position = 0.0f;
    status->step_position = g_stepper.position_steps;
    status->vision_valid = false;
    status->enabled = enabled;
    status->level_confirmed = g_level_confirmed;
#if BALL_BALANCE_SOFT_LIMIT_STEPS > 0
    status->at_soft_limit =
        BallBalanceAbsSteps(g_stepper.position_steps) >=
        BALL_BALANCE_SOFT_LIMIT_STEPS;
#else
    status->at_soft_limit = false;
#endif
}

static void BallBalanceSafeStop(void)
{
    Stepper_Stop(&g_stepper);
    (void) Stepper_Enable(&g_stepper, false);
    g_open_active = false;
    g_open_state_commanded = false;
    g_open_state = BALL_BALANCE_OPEN_IDLE;
}

static Device_Status_e BallBalanceSendUartSpeed(int16_t speed_rpm)
{
    Device_Status_e status = ZDTStepperUART_SetSpeed(&g_stepper_uart,
        speed_rpm, BALL_BALANCE_STEPPER_UART_ACCEL,
        (bool) BALL_BALANCE_STEPPER_UART_SYNC);
    ball_balance_debug_uart_last_status = status;
    if (status == DEVICE_OK) {
        ball_balance_debug_uart_command_count++;
        ball_balance_debug_uart_speed_rpm = speed_rpm;
    }
    return status;
}

static void BallBalanceStopUartSpeed(void)
{
    if (!g_uart_speed_active) {
        return;
    }
    (void) BallBalanceSendUartSpeed(0);
    g_uart_speed_active = false;
    g_last_uart_speed_rpm = 0;
    g_last_uart_command_ms = 0U;
}

static void BallBalanceStartFromCurrentLevel(void)
{
    Stepper_Stop(&g_stepper);
    Stepper_ResetPosition(&g_stepper, 0);
    g_level_confirmed = true;
    g_open_active = false;
    g_open_state_commanded = false;
    g_open_state = BALL_BALANCE_OPEN_IDLE;
    g_open_state_start_ms = 0U;
}

static Device_Status_e BallBalanceStartRelativeMove(int32_t steps)
{
    if (steps == 0) {
        ball_balance_debug_rejected_target_steps = g_stepper.position_steps;
        ball_balance_debug_move_error_count++;
        return DEVICE_ERROR;
    }
    int32_t target_position = g_stepper.position_steps + steps;
#if BALL_BALANCE_SOFT_LIMIT_STEPS > 0
    if (BallBalanceAbsSteps(target_position) >
        BALL_BALANCE_SOFT_LIMIT_STEPS) {
        ball_balance_debug_rejected_target_steps = target_position;
        ball_balance_debug_move_error_count++;
        return DEVICE_ERROR;
    }
#else
    (void) target_position;
#endif

    Stepper_Direction_e direction =
        (steps > 0) ? STEPPER_DIR_UP : STEPPER_DIR_DOWN;
    uint32_t step_count =
        (steps > 0) ? (uint32_t) steps : (uint32_t) -steps;
    return Stepper_Move(&g_stepper, direction, step_count,
        ball_balance_open_speed_sps);
}

static Device_Status_e BallBalanceMoveToPosition(int32_t target_steps)
{
    ball_balance_debug_last_target_steps = target_steps;
    int32_t move_steps = target_steps - g_stepper.position_steps;
    if (move_steps == 0) {
        return DEVICE_OK;
    }

    return BallBalanceStartRelativeMove(move_steps);
}

static void BallBalanceEnterOpenState(
    BallBalance_Open_State_t state, uint32_t now_ms)
{
    g_open_state = state;
    g_open_state_start_ms = now_ms;
    g_open_state_commanded = false;
    g_open_active = state != BALL_BALANCE_OPEN_IDLE;
}

static int32_t BallBalanceOpenStateTarget(BallBalance_Open_State_t state)
{
    switch (state) {
    case BALL_BALANCE_OPEN_POS_GO:
        return ball_balance_open_pos_go_target_steps;
    case BALL_BALANCE_OPEN_NEG_GO:
        return ball_balance_open_neg_go_target_steps;
    case BALL_BALANCE_OPEN_FINAL_BUMP_POS:
        return ball_balance_open_final_bump_pos_target_steps;
    case BALL_BALANCE_OPEN_FINAL_BUMP_NEG:
        return ball_balance_open_final_bump_neg_target_steps;
    case BALL_BALANCE_OPEN_FINAL_HOLD:
        return ball_balance_open_final_hold_target_steps;
    default:
        return 0;
    }
}

static uint32_t BallBalanceOpenStateHoldMs(BallBalance_Open_State_t state)
{
    switch (state) {
    case BALL_BALANCE_OPEN_POS_GO:
        return ball_balance_open_pos_go_hold_ms;
    case BALL_BALANCE_OPEN_NEG_GO:
        return ball_balance_open_neg_go_hold_ms;
    case BALL_BALANCE_OPEN_FINAL_BUMP_POS:
        return ball_balance_open_final_bump_pos_hold_ms;
    case BALL_BALANCE_OPEN_FINAL_BUMP_NEG:
        return ball_balance_open_final_bump_neg_hold_ms;
    default:
        return 0U;
    }
}

static BallBalance_Open_State_t BallBalanceNextOpenState(
    BallBalance_Open_State_t state)
{
    switch (state) {
    case BALL_BALANCE_OPEN_POS_GO:
        return BALL_BALANCE_OPEN_NEG_GO;
    case BALL_BALANCE_OPEN_NEG_GO:
        return BALL_BALANCE_OPEN_FINAL_BUMP_POS;
    case BALL_BALANCE_OPEN_FINAL_BUMP_POS:
        return BALL_BALANCE_OPEN_FINAL_BUMP_NEG;
    case BALL_BALANCE_OPEN_FINAL_BUMP_NEG:
        return BALL_BALANCE_OPEN_FINAL_HOLD;
    default:
        return BALL_BALANCE_OPEN_FINAL_HOLD;
    }
}

static void BallBalanceRunOpenLoop(uint32_t now_ms)
{
#if BALL_BALANCE_OPEN_LOOP_ENABLED
    if (!g_level_confirmed || g_stepper.running) {
        return;
    }

    if (g_open_state == BALL_BALANCE_OPEN_IDLE) {
        BallBalanceEnterOpenState(BALL_BALANCE_OPEN_POS_GO, now_ms);
    }

    if (!g_open_state_commanded) {
        if (BallBalanceMoveToPosition(
                BallBalanceOpenStateTarget(g_open_state)) != DEVICE_OK) {
            g_open_active = false;
            return;
        }
        g_open_state_commanded = true;
        g_open_state_start_ms = now_ms;
        return;
    }

    if (g_open_state == BALL_BALANCE_OPEN_FINAL_HOLD) {
        g_open_active = false;
        g_open_state_commanded = true;
        return;
    }

    if ((now_ms - g_open_state_start_ms) >=
        BallBalanceOpenStateHoldMs(g_open_state)) {
        BallBalanceEnterOpenState(
            BallBalanceNextOpenState(g_open_state), now_ms);
    }
#else
    g_open_active = false;
#endif
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
    if (ZDTStepperUART_Init(&g_stepper_uart,
            BALL_BALANCE_STEPPER_UART_PORT,
            BALL_BALANCE_STEPPER_UART_ADDRESS) != DEVICE_OK) {
        return false;
    }

    (void) Stepper_Enable(&g_stepper, false);
    g_level_confirmed = false;
    g_command_was_enabled = false;
    g_open_active = false;
    g_open_state_commanded = false;
    g_open_state = BALL_BALANCE_OPEN_IDLE;
    g_open_state_start_ms = 0U;
    g_uart_speed_active = false;
    g_last_uart_speed_rpm = 0;
    g_last_uart_command_ms = 0U;
    ball_balance_debug_uart_speed_rpm = 0;
    ball_balance_debug_uart_command_count = 0U;
    ball_balance_debug_uart_last_status = DEVICE_OK;
    g_initialized = true;
    return true;
}

void BallBalanceTask(const BallBalance_Command_t *command,
    uint32_t now_ms, float dt_seconds, BallBalance_Status_t *status)
{
    (void) dt_seconds;

    if (!g_initialized) {
        BallBalanceWriteStatus(status, false);
        return;
    }

    Stepper_Task(&g_stepper, 1U);
    ball_balance_debug_level_key_pressed = false;

    if ((command == NULL) || !command->enabled) {
        g_command_was_enabled = false;
        BallBalanceStopUartSpeed();
        BallBalanceSafeStop();
        BallBalanceWriteStatus(status, false);
        ball_balance_debug_level_confirmed = g_level_confirmed;
        ball_balance_debug_open_active = g_open_active;
        ball_balance_debug_open_state = (uint8_t) g_open_state;
        ball_balance_debug_open_state_commanded = g_open_state_commanded;
        ball_balance_debug_step_position = g_stepper.position_steps;
        return;
    }

    if (command->uart_speed_control_enabled) {
        g_command_was_enabled = false;
        g_level_confirmed = true;
        g_open_active = true;
        g_open_state = BALL_BALANCE_OPEN_IDLE;
        g_open_state_commanded = true;
        Stepper_Stop(&g_stepper);
        (void) Stepper_Enable(&g_stepper, false);
        if (!g_uart_speed_active ||
            (g_last_uart_speed_rpm != command->speed_rpm) ||
            ((now_ms - g_last_uart_command_ms) >=
                BALL_BALANCE_STEPPER_UART_COMMAND_PERIOD_MS)) {
            if (BallBalanceSendUartSpeed(command->speed_rpm) == DEVICE_OK) {
                g_uart_speed_active = true;
                g_last_uart_speed_rpm = command->speed_rpm;
                g_last_uart_command_ms = now_ms;
            }
        }
        BallBalanceWriteStatus(status, true);
        ball_balance_debug_level_confirmed = g_level_confirmed;
        ball_balance_debug_open_active = g_open_active;
        ball_balance_debug_open_state = (uint8_t) g_open_state;
        ball_balance_debug_open_state_commanded = g_open_state_commanded;
        ball_balance_debug_step_position = g_stepper.position_steps;
        return;
    }

    BallBalanceStopUartSpeed();
    if (!g_command_was_enabled) {
        BallBalanceStartFromCurrentLevel();
        g_command_was_enabled = true;
    }

    if (g_level_confirmed) {
        BallBalanceRunOpenLoop(now_ms);
    } else {
        BallBalanceSafeStop();
    }
    BallBalanceWriteStatus(status, g_level_confirmed);
    ball_balance_debug_level_confirmed = g_level_confirmed;
    ball_balance_debug_open_active = g_open_active;
    ball_balance_debug_open_state = (uint8_t) g_open_state;
    ball_balance_debug_open_state_commanded = g_open_state_commanded;
    ball_balance_debug_step_position = g_stepper.position_steps;
}
