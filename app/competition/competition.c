#include "competition.h"

#include <stddef.h>

#include "key.h"
#include "robot_def.h"
#include "ti_msp_dl_config.h"

typedef void (*CompetitionStateHandler)(uint32_t now_ms,
    bool start_event, const LineFollow_Output_t *line_follow,
    Competition_Output_t *output);

static KEY_Device_t g_start_key;
static Competition_State_t g_state;
static uint32_t g_start_time_ms;
static uint32_t g_a_marker_count;
static uint32_t g_key_stable_samples;
static bool g_key_candidate;
static bool g_key_debounced;
static bool g_initialized;

static void CompetitionSafeOutput(Competition_Output_t *output)
{
    *output = (Competition_Output_t) {0};
    output->ball_balance.target_position =
        BALL_BALANCE_TARGET_POSITION;
}

static bool CompetitionReadStartEvent(void)
{
    const bool pressed = KEY_IsPressed(&g_start_key);
    if (pressed != g_key_candidate) {
        g_key_candidate = pressed;
        g_key_stable_samples = 1U;
    } else if (g_key_stable_samples < COMPETITION_KEY_DEBOUNCE_SAMPLES) {
        g_key_stable_samples++;
    }

    if ((g_key_stable_samples >= COMPETITION_KEY_DEBOUNCE_SAMPLES) &&
        (g_key_debounced != g_key_candidate)) {
        g_key_debounced = g_key_candidate;
        return g_key_debounced;
    }
    return false;
}

static void CompetitionDisarmedHandler(uint32_t now_ms,
    bool start_event, const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    (void) now_ms;
    (void) start_event;
    (void) line_follow;
    (void) output;
    g_state = COMPETITION_READY;
}

static void CompetitionReadyHandler(uint32_t now_ms,
    bool start_event, const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    (void) line_follow;
    (void) output;
    if (start_event) {
        g_start_time_ms = now_ms;
        g_a_marker_count = 0U;
        g_state = COMPETITION_RUNNING;
    }
}

static void CompetitionRunningHandler(uint32_t now_ms,
    bool start_event, const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    if (start_event ||
        ((now_ms - g_start_time_ms) >= COMPETITION_TIME_LIMIT_MS)) {
        g_state = COMPETITION_FINISHED;
        return;
    }

    output->line_follow_enabled = true;
    output->ball_balance.enabled = true;
    if (line_follow != NULL) {
        if (line_follow->a_marker_event) {
            g_a_marker_count++;
        }
        if (line_follow->line_valid) {
            output->chassis.vx_mps = line_follow->vx_mps;
            output->chassis.wz_radps = line_follow->wz_radps;
            output->chassis.enabled = true;
        }
    }
}

static void CompetitionFinishedHandler(uint32_t now_ms,
    bool start_event, const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    (void) now_ms;
    (void) line_follow;
    (void) output;
    if (start_event) {
        g_state = COMPETITION_READY;
    }
}

static void CompetitionFaultHandler(uint32_t now_ms,
    bool start_event, const LineFollow_Output_t *line_follow,
    Competition_Output_t *output)
{
    (void) now_ms;
    (void) start_event;
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
    g_state = COMPETITION_DISARMED;
    g_start_time_ms = 0U;
    g_a_marker_count = 0U;
    g_key_stable_samples = 0U;
    g_key_candidate = false;
    g_key_debounced = false;
    g_initialized = KEY_Init(&g_start_key, KEY_GPIO_KEY1_PORT,
        KEY_GPIO_KEY1_PIN, GPIO_PIN_RESET);
    if (!g_initialized) {
        g_state = COMPETITION_FAULT;
    }
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
        g_state_handlers[g_state](now_ms, CompetitionReadStartEvent(),
            line_follow, output);
    } else {
        g_state = COMPETITION_FAULT;
    }

    output->status.state = g_state;
    output->status.elapsed_ms =
        (g_state == COMPETITION_RUNNING)
            ? (now_ms - g_start_time_ms)
            : 0U;
    output->status.a_marker_count = g_a_marker_count;
    output->status.line_valid =
        (line_follow != NULL) && line_follow->line_valid;
    output->status.vision_valid =
        (ball_balance != NULL) && ball_balance->vision_valid;
}
