#ifndef CHASSIS_H
#define CHASSIS_H

#include <stdbool.h>

typedef struct {
    float vx_mps;
    float wz_radps;
    bool enabled;
} Chassis_Command_t;

typedef struct {
    float left_target_counts_s;
    float left_measured_counts_s;
    float right_target_counts_s;
    float right_measured_counts_s;
    bool enabled;
} Chassis_Status_t;

bool ChassisInit(void);
void ChassisTask(const Chassis_Command_t *command,
    float dt_seconds, Chassis_Status_t *status);

#endif
