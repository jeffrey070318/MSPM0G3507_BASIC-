#ifndef GIMBAL_H
#define GIMBAL_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_def.h"

extern volatile Device_Status_e gimbal_stepper_init_status;
extern volatile uint32_t gimbal_stepper_task_count;
extern volatile int32_t gimbal_stepper_position_steps;
extern volatile bool gimbal_stepper_running;
extern volatile Device_Status_e gimbal_key2_init_status;
extern volatile uint32_t gimbal_key2_press_count;
extern volatile uint32_t gimbal_key_active_level;
extern volatile bool gimbal_vision_pipe_request_pending;
extern volatile uint8_t gimbal_pipe_round_trip_target;

void GimbalInit(void);
void GimbalTask(void);
bool GimbalTakeVisionPipeRequest(uint8_t *round_trips);

#endif
