#ifndef GIMBAL_H
#define GIMBAL_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_def.h"

extern volatile Device_Status_e gimbal_stepper_init_status;
extern volatile uint32_t gimbal_stepper_task_count;
extern volatile int32_t gimbal_stepper_position_steps;
extern volatile bool gimbal_stepper_running;

void GimbalInit(void);
void GimbalTask(void);

#endif
