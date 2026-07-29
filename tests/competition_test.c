#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "competition.h"
#include "key.h"
#include "robot_def.h"

GPIO_TypeDef test_key_port;

static bool key_pressed;
static bool key_init_ok = true;

bool KEY_Init(KEY_Device_t *key, GPIO_TypeDef *gpio_port,
    uint32_t gpio_pin, GPIO_PinState active_state)
{
    assert(key != NULL);
    assert(gpio_port == &test_key_port);
    assert(gpio_pin == (1UL << 0U));
    assert(active_state == GPIO_PIN_RESET);
    return key_init_ok;
}

bool KEY_IsPressed(KEY_Device_t *key)
{
    assert(key != NULL);
    return key_pressed;
}

int main(void)
{
    LineFollow_Output_t line = {0};
    BallBalance_Status_t balance = {0};
    Competition_Output_t output = {0};

    assert(CompetitionInit());
    CompetitionTask(0U, true, &line, &balance, &output);
    assert(output.status.state == COMPETITION_READY);
    assert(!output.chassis.enabled);
    assert(!output.ball_balance.enabled);

    key_pressed = true;
    for (uint32_t i = 1U; i <= COMPETITION_KEY_DEBOUNCE_SAMPLES; i++) {
        CompetitionTask(i * 5U, true, &line, &balance, &output);
    }
    assert(output.status.state == COMPETITION_RUNNING);
    assert(!output.chassis.enabled);

    line = (LineFollow_Output_t) {
        .vx_mps = 0.15f,
        .wz_radps = -0.2f,
        .line_valid = true,
    };
    CompetitionTask(20U, true, &line, &balance, &output);
    assert(output.status.state == COMPETITION_RUNNING);
    assert(output.line_follow_enabled);
    assert(output.chassis.enabled);
    assert(output.chassis.vx_mps == line.vx_mps);
    assert(output.chassis.wz_radps == line.wz_radps);
    assert(output.ball_balance.enabled);

    line.line_valid = false;
    CompetitionTask(25U, true, &line, &balance, &output);
    assert(output.status.state == COMPETITION_RUNNING);
    assert(!output.chassis.enabled);
    assert(output.line_follow_enabled);

    CompetitionTask(15U + COMPETITION_TIME_LIMIT_MS,
        true, &line, &balance, &output);
    assert(output.status.state == COMPETITION_FINISHED);
    assert(!output.chassis.enabled);
    assert(!output.ball_balance.enabled);

    key_init_ok = false;
    assert(!CompetitionInit());
    CompetitionTask(0U, false, &line, &balance, &output);
    assert(output.status.state == COMPETITION_FAULT);
    assert(!output.chassis.enabled);
    return 0;
}
