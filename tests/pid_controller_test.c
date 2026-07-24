#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "pid.h"

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static void AssertNear(float actual, float expected)
{
    assert(AbsFloat(actual - expected) < 0.0001f);
}

static PID_Config_t DefaultConfig(void)
{
    PID_Config_t config = {
        .kp = 0.0f,
        .ki = 0.0f,
        .kd = 0.0f,
        .output_limit = 100.0f,
        .integral_limit = 100.0f,
        .deadband = 0.0f,
        .derivative_on_measurement = false,
    };
    return config;
}

int main(void)
{
    PID_Controller_t pid;
    PID_Config_t config = DefaultConfig();

    config.kp = 0.5f;
    assert(PID_ControllerInit(&pid, &config));
    AssertNear(PID_ControllerUpdate(&pid, 10.0f, 4.0f, 0.1f), 3.0f);

    config = DefaultConfig();
    config.ki = 2.0f;
    config.integral_limit = 1.5f;
    assert(PID_ControllerInit(&pid, &config));
    AssertNear(PID_ControllerUpdate(&pid, 1.0f, 0.0f, 0.5f), 1.0f);
    AssertNear(PID_ControllerUpdate(&pid, 1.0f, 0.0f, 0.5f), 1.5f);

    config = DefaultConfig();
    config.kp = 10.0f;
    config.ki = 1.0f;
    config.output_limit = 1.0f;
    assert(PID_ControllerInit(&pid, &config));
    AssertNear(PID_ControllerUpdate(&pid, 1.0f, 0.0f, 1.0f), 1.0f);
    AssertNear(pid.integral, 0.0f);

    config = DefaultConfig();
    config.kd = 1.0f;
    config.derivative_on_measurement = true;
    assert(PID_ControllerInit(&pid, &config));
    AssertNear(PID_ControllerUpdate(&pid, 0.0f, 0.0f, 0.5f), 0.0f);
    AssertNear(PID_ControllerUpdate(&pid, 0.0f, 2.0f, 0.5f), -4.0f);

    config = DefaultConfig();
    config.kp = 1.0f;
    config.deadband = 0.5f;
    assert(PID_ControllerInit(&pid, &config));
    AssertNear(PID_ControllerUpdate(&pid, 0.4f, 0.0f, 0.1f), 0.0f);

    PID_ControllerSetTunings(&pid, 2.0f, 0.0f, 0.0f);
    AssertNear(PID_ControllerUpdate(&pid, 1.0f, 0.0f, 0.1f), 2.0f);
    PID_ControllerReset(&pid);
    AssertNear(pid.integral, 0.0f);
    AssertNear(pid.output, 0.0f);

    AssertNear(PID_ControllerUpdate(&pid, 1.0f, 0.0f, 0.0f), 0.0f);

    config.output_limit = -1.0f;
    assert(!PID_ControllerInit(&pid, &config));
    assert(!PID_ControllerInit(NULL, &config));

    return 0;
}
