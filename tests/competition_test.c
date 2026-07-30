#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "competition.h"
#include "key.h"
#include "robot_def.h"

GPIO_TypeDef test_key_port;

static bool key1_pressed;
static bool key2_pressed;
static bool key_init_ok = true;
static uint32_t key_init_count;

extern volatile uint8_t competition_debug_key_bits;
extern volatile Competition_State_t competition_debug_state;

bool KEY_Init(KEY_Device_t *key, GPIO_TypeDef *gpio_port,
    uint32_t gpio_pin, GPIO_PinState active_state)
{
    assert(key != NULL);
    assert(gpio_port == &test_key_port);
    assert((gpio_pin == (1UL << 0U)) || (gpio_pin == (1UL << 1U)));
    assert(active_state == GPIO_PIN_RESET);
    key->marker = gpio_pin;
    key_init_count++;
    return key_init_ok;
}

bool KEY_IsPressed(KEY_Device_t *key)
{
    assert(key != NULL);
    if (key->marker == (1UL << 0U)) {
        return key1_pressed;
    }
    assert(key->marker == (1UL << 1U));
    return key2_pressed;
}

static void PressModeKey(bool line_mode, uint32_t *now_ms,
    LineFollow_Output_t *line, BallBalance_Status_t *balance,
    Competition_Output_t *output)
{
    key1_pressed = line_mode;
    key2_pressed = !line_mode;
    for (uint32_t i = 0U; i < COMPETITION_KEY_DEBOUNCE_SAMPLES; i++) {
        *now_ms += 5U;
        CompetitionTask(*now_ms, true, line, balance, output);
    }
}

int main(void)
{
    LineFollow_Output_t line = {0};
    BallBalance_Status_t balance = {0};
    Competition_Output_t output = {0};
    uint32_t now_ms = 0U;

    assert(CompetitionInit());
    assert(key_init_count == 2U);
    assert(competition_debug_key_bits == 0U);
    assert(competition_debug_state == COMPETITION_DISARMED);
    CompetitionTask(now_ms, true, &line, &balance, &output);
    assert(output.status.state == COMPETITION_READY);
    assert(competition_debug_state == COMPETITION_READY);
    assert(output.status.mode == COMPETITION_MODE_NONE);

    PressModeKey(true, &now_ms, &line, &balance, &output);
    assert(output.status.state == COMPETITION_RUNNING);
    assert(output.status.mode == COMPETITION_MODE_LINE_FOLLOW);
    assert(competition_debug_key_bits == 0x01U);
    assert(competition_debug_state == COMPETITION_RUNNING);

    line = (LineFollow_Output_t) {
        .vx_mps = 0.15f,
        .wz_radps = -0.2f,
        .line_valid = true,
    };
    now_ms += 5U;
    CompetitionTask(now_ms, true, &line, &balance, &output);
    assert(output.line_follow_enabled);
    assert(output.chassis.enabled);
    assert(!output.ball_balance.enabled);
    assert(output.chassis.vx_mps == line.vx_mps);
    assert(output.chassis.wz_radps == line.wz_radps);

    line.line_valid = false;
    now_ms += 5U;
    CompetitionTask(now_ms, true, &line, &balance, &output);
    assert(output.status.state == COMPETITION_RUNNING);
    assert(!output.chassis.enabled);
    assert(output.line_follow_enabled);

    CompetitionTask(now_ms + COMPETITION_TIME_LIMIT_MS,
        true, &line, &balance, &output);
    assert(output.status.state == COMPETITION_FINISHED);
    assert(!output.chassis.enabled);
    assert(!output.ball_balance.enabled);

    key1_pressed = false;
    key2_pressed = false;
    key_init_count = 0U;
    now_ms = 0U;
    assert(CompetitionInit());
    CompetitionTask(now_ms, true, &line, &balance, &output);
    PressModeKey(false, &now_ms, &line, &balance, &output);
    assert(output.status.state == COMPETITION_RUNNING);
    assert(output.status.mode == COMPETITION_MODE_BALL_BALANCE);
    assert(competition_debug_key_bits == 0x02U);
    assert(competition_debug_state == COMPETITION_RUNNING);
    now_ms += 5U;
    CompetitionTask(now_ms, true, &line, &balance, &output);
    assert(!output.line_follow_enabled);
    assert(!output.chassis.enabled);
    assert(output.ball_balance.enabled);

    key_init_ok = false;
    assert(!CompetitionInit());
    assert(competition_debug_state == COMPETITION_FAULT);
    CompetitionTask(0U, false, &line, &balance, &output);
    assert(output.status.state == COMPETITION_FAULT);
    assert(!output.chassis.enabled);
    return 0;
}
