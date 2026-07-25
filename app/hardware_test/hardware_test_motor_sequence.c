#include "hardware_test_motor_sequence.h"

#include <stddef.h>

#define MOTOR_WAIT_TICKS    200U
#define MOTOR_FORWARD_TICKS 100U
#define MOTOR_PAUSE_TICKS   200U
#define MOTOR_REVERSE_TICKS 100U
#define MOTOR_TEST_OUTPUT   0.15f

static void HardwareTestMotorSequence_Complete(
    HardwareTestMotorSequence_t *sequence)
{
    sequence->stage = HARDWARE_TEST_MOTOR_COMPLETE;
    sequence->stage_ticks = 0U;
    sequence->output = 0.0f;
    sequence->stop_latched = true;
}

void HardwareTestMotorSequence_Init(HardwareTestMotorSequence_t *sequence)
{
    if (sequence == NULL) {
        return;
    }

    sequence->stage = HARDWARE_TEST_MOTOR_WAIT;
    sequence->stage_ticks = 0U;
    sequence->output = 0.0f;
    sequence->stop_latched = false;
}

void HardwareTestMotorSequence_Step(HardwareTestMotorSequence_t *sequence)
{
    if (sequence == NULL) {
        return;
    }

    switch (sequence->stage) {
        case HARDWARE_TEST_MOTOR_WAIT:
            sequence->stage_ticks++;
            if (sequence->stage_ticks >= MOTOR_WAIT_TICKS) {
                sequence->stage = HARDWARE_TEST_MOTOR_FORWARD;
                sequence->stage_ticks = 0U;
                sequence->output = MOTOR_TEST_OUTPUT;
            }
            break;

        case HARDWARE_TEST_MOTOR_FORWARD:
            sequence->stage_ticks++;
            if (sequence->stage_ticks >= MOTOR_FORWARD_TICKS) {
                sequence->stage = HARDWARE_TEST_MOTOR_PAUSE;
                sequence->stage_ticks = 0U;
                sequence->output = 0.0f;
            }
            break;

        case HARDWARE_TEST_MOTOR_PAUSE:
            sequence->stage_ticks++;
            if (sequence->stage_ticks >= MOTOR_PAUSE_TICKS) {
                sequence->stage = HARDWARE_TEST_MOTOR_REVERSE;
                sequence->stage_ticks = 0U;
                sequence->output = -MOTOR_TEST_OUTPUT;
            }
            break;

        case HARDWARE_TEST_MOTOR_REVERSE:
            sequence->stage_ticks++;
            if (sequence->stage_ticks >= MOTOR_REVERSE_TICKS) {
                HardwareTestMotorSequence_Complete(sequence);
            }
            break;

        case HARDWARE_TEST_MOTOR_COMPLETE:
        default:
            HardwareTestMotorSequence_Complete(sequence);
            break;
    }
}
