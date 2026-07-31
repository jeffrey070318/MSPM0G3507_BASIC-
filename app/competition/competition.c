#include "competition.h"

#include <stddef.h>
#include <stdint.h>

#include "bsp_encoder.h"
#include "key.h"
#include "robot_def.h"
#include "ti_msp_dl_config.h"

typedef struct {
    KEY_Device_t device;
    uint32_t stable_samples;
    bool candidate;
    bool debounced;
} Competition_Key_t;

typedef void (*CompetitionStateHandler)(uint32_t now_ms,
    Competition_Mode_t requested_mode,
    const LineFollow_Output_t *line_follow,
    Competition_Output_t *output);

static Competition_Key_t g_line_key;
static Competition_Key_t g_balance_key;
static Competition_State_t g_state;
static Competition_Mode_t g_mode;
static uint32_t g_start_time_ms;
static uint32_t g_a_marker_count;
static bool g_initialized;

volatile uint8_t competition_debug_key_bits;
volatile uint8_t competition_debug_key_raw_bits;
volatile uint8_t competition_debug_key_seen_bits;
volatile Competition_State_t competition_debug_state;
volatile int32_t competition_debug_left_encoder_start;
volatile int32_t competition_debug_right_encoder_start;
volatile int32_t competition_debug_left_encoder_total;
volatile int32_t competition_debug_right_encoder_total;
volatile int32_t competition_debug_left_encoder_traveled;
volatile int32_t competition_debug_right_encoder_traveled;
volatile int32_t competition_debug_encoder_average_traveled;
volatile int32_t competition_debug_encoder_stop_counts =
    COMPETITION_KEY1_ENCODER_STOP_COUNTS;
volatile bool competition_debug_encoder_stop_latched;

static int32_t CompetitionAbsDelta(int32_t current, int32_t start)
{
    int64_t delta = (int64_t) current - (int64_t) start;
    if (delta < 0) {
        delta = -delta;
    }
    return (delta > INT32_MAX) ? INT32_MAX : (int32_t) delta;
}

static void CompetitionResetEncoderStop(void)
{
    competition_debug_left_encoder_start =
        Encoder_Get_Total(&hencoder_left);
    competition_debug_right_encoder_start =
        Encoder_Get_Total(&hencoder_right);
    competition_debug_left_encoder_total =
        competition_debug_left_encoder_start;
    competition_debug_right_encoder_total =
        competition_debug_right_encoder_start;
    competition_debug_left_encoder_traveled = 0;
    competition_debug_right_encoder_traveled = 0;
    competition_debug_encoder_average_traveled = 0;
    competition_debug_encoder_stop_latched = false;
}

static void CompetitionUpdateEncoderStop(void)
{
    competition_debug_left_encoder_total =
        Encoder_Get_Total(&hencoder_left);
    competition_debug_right_encoder_total =
        Encoder_Get_Total(&hencoder_right);
    competition_debug_left_encoder_traveled = CompetitionAbsDelta(
        competition_debug_left_encoder_total,
        competition_debug_left_encoder_start);
    competition_debug_right_encoder_traveled = CompetitionAbsDelta(
        competition_debug_right_encoder_total,
        competition_debug_right_encoder_start);
    competition_debug_encoder_average_traveled =
        (competition_debug_left_encoder_traveled +
            competition_debug_right_encoder_traveled) / 2;
}

static bool CompetitionEncoderStopReached(void)
{
#if COMPETITION_KEY1_ENCODER_STOP_ENABLED
    CompetitionUpdateEncoderStop();
    if ((competition_debug_encoder_stop_counts > 0) &&
        (competition_debug_encoder_average_traveled >=
            competition_debug_encoder_stop_counts)) {
        competition_debug_encoder_stop_latched = true;
    }
#endif
    return competition_debug_encoder_stop_latched;
}

static void CompetitionSafeOutput(Competition_Output_t *output)
{
    *output = (Competition_Output_t) {0};
    output->ball_balance.target_position =
        BALL_BALANCE_TARGET_POSITION;
}

static bool CompetitionUpdateKeyEvent(
    Competition_Key_t *key, bool pressed)
{
    if (pressed != key->candidate) {
        key->candidate = pressed;
        key->stable_samples = 1U;
    } else if (key->stable_samples < COMPETITION_KEY_DEBOUNCE_SAMPLES) {
        key->stable_samples++;
    }

    if ((key->stable_samples >= COMPETITION_KEY_DEBOUNCE_SAMPLES) &&
        (key->debounced != key->candidate)) {
        key->debounced = key->candidate;
        return key->debounced;
    }
    return false;
}

static Competition_Mode_t CompetitionReadModeRequest(void)
{
    competition_debug_key_raw_bits =
        (DL_GPIO_readPins(KEY_GPIO_KEY1_PORT, KEY_GPIO_KEY1_PIN) != 0U ? 0x01U : 0U) |
        (DL_GPIO_readPins(KEY_GPIO_KEY2_PORT, KEY_GPIO_KEY2_PIN) != 0U ? 0x02U : 0U) |
        (DL_GPIO_readPins(KEY_GPIO_KEY3_PORT, KEY_GPIO_KEY3_PIN) != 0U ? 0x04U : 0U);
    const bool line_pressed = KEY_IsPressed(&g_line_key.device);
    const bool balance_pressed = KEY_IsPressed(&g_balance_key.device);
    competition_debug_key_bits =
        (line_pressed ? 0x01U : 0U) |
        (balance_pressed ? 0x02U : 0U);

    const bool line_event =
        CompetitionUpdateKeyEvent(&g_line_key, line_pressed);
    const bool balance_event =
        CompetitionUpdateKeyEvent(&g_balance_key, balance_pressed);
    competition_debug_key_seen_bits |=
        (line_event ? 0x01U : 0U) | (balance_event ? 0x02U : 0U);
    if (line_event) {
        return COMPETITION_MODE_LINE_FOLLOW;
    }
    if (balance_event) {
        return COMPETITION_MODE_BALL_BALANCE;
    }
    return COMPETITION_MODE_NONE;
}

static void CompetitionDisarmedHandler(uint32_t now_ms,
    Competition_Mode_t requested_mode,
    const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    (void) now_ms;
    (void) requested_mode;
    (void) line_follow;
    (void) output;
    g_state = COMPETITION_READY;
}

static void CompetitionReadyHandler(uint32_t now_ms,
    Competition_Mode_t requested_mode,
    const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    (void) line_follow;
    (void) output;
    if (requested_mode != COMPETITION_MODE_NONE) {
        g_start_time_ms = now_ms;
        g_a_marker_count = 0U;
        g_mode = requested_mode;
        if (g_mode == COMPETITION_MODE_LINE_FOLLOW) {
            CompetitionResetEncoderStop();
        }
        g_state = COMPETITION_RUNNING;
    }
}

static void CompetitionRunningHandler(uint32_t now_ms,
    Competition_Mode_t requested_mode,
    const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    if ((requested_mode != COMPETITION_MODE_NONE) ||
        ((now_ms - g_start_time_ms) >= COMPETITION_TIME_LIMIT_MS)) {
        g_state = COMPETITION_FINISHED;
        return;
    }

    if (g_mode == COMPETITION_MODE_BALL_BALANCE) {
        output->ball_balance.enabled = true;
        return;
    }

    if (g_mode != COMPETITION_MODE_LINE_FOLLOW) {
        return;
    }

    if (CompetitionEncoderStopReached()) {
        g_state = COMPETITION_FINISHED;
        return;
    }

    output->line_follow_enabled = true;
    if (line_follow != NULL) {
        if (line_follow->a_marker_event) {
            g_a_marker_count++;
            if (g_a_marker_count >= COMPETITION_STOP_A_MARKER_COUNT) {
                g_state = COMPETITION_FINISHED;
                return;
            }
        }
        if (line_follow->line_valid) {
            output->chassis.vx_mps = line_follow->vx_mps;
            output->chassis.wz_radps = line_follow->wz_radps;
            output->chassis.enabled = true;
        }
    }
}

static void CompetitionFinishedHandler(uint32_t now_ms,
    Competition_Mode_t requested_mode,
    const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    (void) now_ms;
    (void) line_follow;
    (void) output;
    if (requested_mode != COMPETITION_MODE_NONE) {
        g_mode = COMPETITION_MODE_NONE;
        g_state = COMPETITION_READY;
    }
}

static void CompetitionFaultHandler(uint32_t now_ms,
    Competition_Mode_t requested_mode,
    const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    (void) now_ms;
    (void) requested_mode;
    (void) line_follow;
    (void) output;
}

static const CompetitionStateHandler g_state_handlers[
    COMPETITION_STATE_COUNT] = {
    [COMPETITION_DISARMED] = CompetitionDisarmedHandler,
    [COMPETITION_READY] = CompetitionReadyHandler,
    [COMPETITION_RUNNING] = CompetitionRunningHandler,
    [COMPETITION_FINISHED] = CompetitionFinishedHandler,
    [COMPETITION_FAULT] = CompetitionFaultHandler,
};

bool CompetitionInit(void)
{
    g_line_key = (Competition_Key_t) {0};
    g_balance_key = (Competition_Key_t) {0};
    g_state = COMPETITION_DISARMED;
    g_mode = COMPETITION_MODE_NONE;
    g_start_time_ms = 0U;
    g_a_marker_count = 0U;
    CompetitionResetEncoderStop();
    competition_debug_key_bits = 0U;
    competition_debug_key_raw_bits = 0U;
    competition_debug_key_seen_bits = 0U;
    competition_debug_state = COMPETITION_DISARMED;

    const bool line_key_ready = KEY_Init(&g_line_key.device,
        KEY_GPIO_KEY1_PORT, KEY_GPIO_KEY1_PIN, GPIO_PIN_RESET);
    const bool balance_key_ready = KEY_Init(&g_balance_key.device,
        KEY_GPIO_KEY2_PORT, KEY_GPIO_KEY2_PIN, GPIO_PIN_RESET);
    g_initialized = line_key_ready && balance_key_ready;
    if (!g_initialized) {
        g_state = COMPETITION_FAULT;
    }
    competition_debug_state = g_state;
    return g_initialized;
}

void CompetitionTask(uint32_t now_ms, bool app_ready,
    const LineFollow_Output_t *line_follow,
    const BallBalance_Status_t *ball_balance,
    Competition_Output_t *output)
{
    if (output == NULL) {
        return;
    }

    CompetitionSafeOutput(output);
    if (!g_initialized || !app_ready) {
        g_state = COMPETITION_FAULT;
    } else if (g_state < COMPETITION_STATE_COUNT) {
        g_state_handlers[g_state](now_ms, CompetitionReadModeRequest(),
            line_follow, output);
    } else {
        g_state = COMPETITION_FAULT;
    }

    output->status.state = g_state;
    output->status.mode = g_mode;
    output->status.elapsed_ms =
        (g_state == COMPETITION_RUNNING)
            ? (now_ms - g_start_time_ms)
            : 0U;
    output->status.a_marker_count = g_a_marker_count;
    output->status.line_valid =
        (line_follow != NULL) && line_follow->line_valid;
    output->status.vision_valid =
        (ball_balance != NULL) && ball_balance->vision_valid;
    competition_debug_state = g_state;
}
