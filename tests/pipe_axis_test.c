#include <assert.h>
#include <stdint.h>

#include "pipe_axis.h"

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

int main(void)
{
    PipeAxis_t axis = {0};
    assert(PipeAxis_Init(&axis, NULL) == DEVICE_OK);
    assert(axis.initialized);
    assert(AbsFloat(PipeAxis_GetThetaRad(&axis) - 1.5707963f) < 0.01f);
    assert(AbsFloat(PipeAxis_GetJointXmm(&axis)) < 0.5f);
    assert(PipeAxis_GetEndYmm(&axis) > 89.0f);
    assert(PipeAxis_GetEndYmm(&axis) < 92.0f);

    int32_t pulses = 123;
    assert(PipeAxis_DeltaMmToPulses(&axis, 1.0f, &pulses) ==
        DEVICE_ERROR);
    assert(pulses == 0);

    PipeAxis_ResetPosition(&axis, -800);
    assert(AbsFloat(PipeAxis_GetThetaRad(&axis) - 0.7853981f) < 0.01f);
    assert(PipeAxis_DeltaMmToPulses(&axis, 1.0f, &pulses) == DEVICE_OK);
    assert(pulses != 0);
    const int32_t old_position = axis.position_pulses;
    PipeAxis_ApplyPulseFeedback(&axis, pulses);
    assert(axis.position_pulses == old_position + pulses);

    PipeAxis_Config_t invalid = axis.config;
    invalid.rod_mm = invalid.crank_mm;
    assert(PipeAxis_Init(&axis, &invalid) == DEVICE_ERROR);
    assert(!axis.initialized);
    return 0;
}
