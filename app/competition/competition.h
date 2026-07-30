#ifndef COMPETITION_H
#define COMPETITION_H

#include <stdbool.h>
#include <stdint.h>

#include "ball_balance.h"
#include "chassis.h"
#include "line_follow.h"

typedef enum {
    COMPETITION_DISARMED = 0,
    COMPETITION_READY,
    COMPETITION_RUNNING,
    COMPETITION_FINISHED,
    COMPETITION_FAULT,
    COMPETITION_STATE_COUNT,
} Competition_State_t;

typedef enum {
    COMPETITION_MODE_NONE = 0,
    COMPETITION_MODE_LINE_FOLLOW,
    COMPETITION_MODE_BALL_BALANCE,
} Competition_Mode_t;

typedef struct {
    Competition_State_t state;
    Competition_Mode_t mode;
    uint32_t elapsed_ms;
    uint32_t a_marker_count;
    bool line_valid;
    bool vision_valid;
} Competition_Status_t;

typedef struct {
    bool line_follow_enabled;
    Chassis_Command_t chassis;
    BallBalance_Command_t ball_balance;
    Competition_Status_t status;
} Competition_Output_t;

bool CompetitionInit(void);
void CompetitionTask(uint32_t now_ms, bool app_ready,
    const LineFollow_Output_t *line_follow,
    const BallBalance_Status_t *ball_balance,
    Competition_Output_t *output);

#endif
