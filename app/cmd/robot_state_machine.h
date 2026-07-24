#ifndef ROBOT_STATE_MACHINE_H
#define ROBOT_STATE_MACHINE_H

typedef enum {
    ROBOT_STATE_A = 0,
    ROBOT_STATE_B,
    ROBOT_STATE_COUNT,
} Robot_State_e;

void RobotStateMachineInit(void);
void RobotStateMachineTask(void);
void RobotStateMachineSetState(Robot_State_e state);
Robot_State_e RobotStateMachineGetState(void);

#endif
