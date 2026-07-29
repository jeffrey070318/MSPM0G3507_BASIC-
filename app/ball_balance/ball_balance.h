#ifndef BALL_BALANCE_H
#define BALL_BALANCE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float target_position;
    bool enabled;
} BallBalance_Command_t;

typedef struct {
    float measured_position;
    int32_t step_position;
    bool vision_valid;
    bool enabled;
    bool at_soft_limit;
} BallBalance_Status_t;

bool BallBalanceInit(void);
void BallBalanceTask(const BallBalance_Command_t *command,
    uint32_t now_ms, float dt_seconds, BallBalance_Status_t *status);

#endif
