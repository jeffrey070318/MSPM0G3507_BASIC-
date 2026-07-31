#include "ball_balance.h"

#include <stddef.h>

#include "key.h"
#include "robot_def.h"
#include "stepper.h"
#include "ti_msp_dl_config.h"
#include "vision.h"

typedef enum {
    BALL_BALANCE_OPEN_IDLE = 0,
    BALL_BALANCE_OPEN_POS_GO,
    BALL_BALANCE_OPEN_POS_BRAKE,
    BALL_BALANCE_OPEN_POS_SETTLE,
    BALL_BALANCE_OPEN_NEG_GO,
    BALL_BALANCE_OPEN_NEG_BRAKE,
    BALL_BALANCE_OPEN_FINAL_HOLD,
} BallBalance_Open_State_t;

static Vision_Device_t g_vision;
static Stepper_Device_t g_stepper;
static KEY_Device_t g_level_key;
static uint32_t g_level_key_stable_samples;
static bool g_level_key_candidate;
static bool g_level_key_debounced;
static bool g_level_confirmed;
static bool g_open_active;
static bool g_open_state_commanded;
static BallBalance_Open_State_t g_open_state;
static uint32_t g_open_state_start_ms;
static bool g_initialized;

volatile int32_t ball_balance_open_pos_go_target_steps =
    BALL_BALANCE_OPEN_POS_GO_TARGET_STEPS;
volatile uint32_t ball_balance_open_pos_go_hold_ms =
    BALL_BALANCE_OPEN_POS_GO_HOLD_MS;
volatile int32_t ball_balance_open_pos_brake_target_steps =
    BALL_BALANCE_OPEN_POS_BRAKE_TARGET_STEPS;
volatile uint32_t ball_balance_open_pos_brake_hold_ms =
    BALL_BALANCE_OPEN_POS_BRAKE_HOLD_MS;
volatile int32_t ball_balance_open_pos_settle_target_steps =
    BALL_BALANCE_OPEN_POS_SETTLE_TARGET_STEPS;
volatile uint32_t ball_balance_open_pos_settle_hold_ms =
    BALL_BALANCE_OPEN_POS_SETTLE_HOLD_MS;
volatile int32_t ball_balance_open_neg_go_target_steps =
    BALL_BALANCE_OPEN_NEG_GO_TARGET_STEPS;
volatile uint32_t ball_balance_open_neg_go_hold_ms =
    BALL_BALANCE_OPEN_NEG_GO_HOLD_MS;
volatile int32_t ball_balance_open_neg_brake_target_steps =
    BALL_BALANCE_OPEN_NEG_BRAKE_TARGET_STEPS;
volatile uint32_t ball_balance_open_neg_brake_hold_ms =
    BALL_BALANCE_OPEN_NEG_BRAKE_HOLD_MS;
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

static bool BallBalanceUpdateLevelKey(void)
{
    const bool pressed = KEY_IsPressed(&g_level_key);
    ball_balance_debug_level_key_pressed = pressed;
    if (pressed != g_level_key_candidate) {
        g_level_key_candidate = pressed;
        g_level_key_stable_samples = 1U;
    } else if (g_level_key_stable_samples <
               BALL_BALANCE_LEVEL_KEY_DEBOUNCE_SAMPLES) {
        g_level_key_stable_samples++;
    }

    if ((g_level_key_stable_samples >=
            BALL_BALANCE_LEVEL_KEY_DEBOUNCE_SAMPLES) &&
        (g_level_key_debounced != g_level_key_candidate)) {
        g_level_key_debounced = g_level_key_candidate;
        return g_level_key_debounced;
    }
    return false;
}

static void BallBalanceConfirmLevel(void)
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
    case BALL_BALANCE_OPEN_POS_BRAKE:
        return ball_balance_open_pos_brake_target_steps;
    case BALL_BALANCE_OPEN_POS_SETTLE:
        return ball_balance_open_pos_settle_target_steps;
    case BALL_BALANCE_OPEN_NEG_GO:
        return ball_balance_open_neg_go_target_steps;
    case BALL_BALANCE_OPEN_NEG_BRAKE:
        return ball_balance_open_neg_brake_target_steps;
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
    case BALL_BALANCE_OPEN_POS_BRAKE:
        return ball_balance_open_pos_brake_hold_ms;
    case BALL_BALANCE_OPEN_POS_SETTLE:
        return ball_balance_open_pos_settle_hold_ms;
    case BALL_BALANCE_OPEN_NEG_GO:
        return ball_balance_open_neg_go_hold_ms;
    case BALL_BALANCE_OPEN_NEG_BRAKE:
        return ball_balance_open_neg_brake_hold_ms;
    default:
        return 0U;
    }
}

static BallBalance_Open_State_t BallBalanceNextOpenState(
    BallBalance_Open_State_t state)
{
    switch (state) {
    case BALL_BALANCE_OPEN_POS_GO:
        return BALL_BALANCE_OPEN_POS_BRAKE;
    case BALL_BALANCE_OPEN_POS_BRAKE:
        return BALL_BALANCE_OPEN_POS_SETTLE;
    case BALL_BALANCE_OPEN_POS_SETTLE:
        return BALL_BALANCE_OPEN_NEG_GO;
    case BALL_BALANCE_OPEN_NEG_GO:
        return BALL_BALANCE_OPEN_NEG_BRAKE;
    case BALL_BALANCE_OPEN_NEG_BRAKE:
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
    if (!KEY_Init(&g_level_key, KEY_GPIO_KEY3_PORT, KEY_GPIO_KEY3_PIN,
            (GPIO_PinState) BALL_BALANCE_LEVEL_KEY_ACTIVE_STATE)) {
        return false;
    }

    (void) Stepper_Enable(&g_stepper, false);
    g_level_key_stable_samples = 0U;
    g_level_key_candidate = false;
    g_level_key_debounced = false;
    g_level_confirmed = false;
    g_open_active = false;
    g_open_state_commanded = false;
    g_open_state = BALL_BALANCE_OPEN_IDLE;
    g_open_state_start_ms = 0U;
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
    if (BallBalanceUpdateLevelKey()) {
        BallBalanceConfirmLevel();
    }

    /* No camera is required for the manual level bounce fallback. */
    if ((command == NULL) || !command->enabled) {
        BallBalanceSafeStop();
        BallBalanceWriteStatus(status, false);
        ball_balance_debug_level_confirmed = g_level_confirmed;
        ball_balance_debug_open_active = g_open_active;
        ball_balance_debug_open_state = (uint8_t) g_open_state;
        ball_balance_debug_open_state_commanded = g_open_state_commanded;
        ball_balance_debug_step_position = g_stepper.position_steps;
        return;
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
