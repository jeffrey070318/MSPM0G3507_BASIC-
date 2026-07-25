#include <assert.h>

#include "hardware_test_motor_sequence.h"

static void StepMany(HardwareTestMotorSequence_t *sequence, unsigned count)
{
    for (unsigned i = 0U; i < count; ++i) {
        HardwareTestMotorSequence_Step(sequence);
    }
}

int main(void)
{
    HardwareTestMotorSequence_t sequence;
    HardwareTestMotorSequence_Init(&sequence);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_WAIT);
    assert(sequence.output == 0.0f);
    assert(!sequence.stop_latched);

    StepMany(&sequence, 199U);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_WAIT);
    HardwareTestMotorSequence_Step(&sequence);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_FORWARD);
    assert(sequence.output == 0.50f);

    StepMany(&sequence, 1000U);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_FORWARD);
    assert(sequence.output == 0.50f);
    assert(!sequence.stop_latched);

    sequence.stage = (HardwareTestMotorStage_e) 99;
    sequence.stop_latched = false;
    HardwareTestMotorSequence_Step(&sequence);
    assert(sequence.stage == HARDWARE_TEST_MOTOR_COMPLETE);
    assert(sequence.output == 0.0f);
    assert(sequence.stop_latched);

    HardwareTestMotorSequence_Init(NULL);
    HardwareTestMotorSequence_Step(NULL);
    return 0;
}
