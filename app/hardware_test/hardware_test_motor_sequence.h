#ifndef HARDWARE_TEST_MOTOR_SEQUENCE_H
#define HARDWARE_TEST_MOTOR_SEQUENCE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HARDWARE_TEST_MOTOR_WAIT = 0,
    HARDWARE_TEST_MOTOR_FORWARD,
    HARDWARE_TEST_MOTOR_PAUSE,
    HARDWARE_TEST_MOTOR_REVERSE,
    HARDWARE_TEST_MOTOR_COMPLETE,
} HardwareTestMotorStage_e;

typedef struct {
    HardwareTestMotorStage_e stage;
    uint16_t stage_ticks;
    float output;
    bool stop_latched;
} HardwareTestMotorSequence_t;

void HardwareTestMotorSequence_Init(HardwareTestMotorSequence_t *sequence);
void HardwareTestMotorSequence_Step(HardwareTestMotorSequence_t *sequence);

#endif
