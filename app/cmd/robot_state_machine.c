#include "robot_state_machine.h"

// 预留模版

typedef void (*RobotStateHandler)(void);

static Robot_State_e current_state = ROBOT_STATE_A;

static void RobotStateA(void)
{
}

static void RobotStateB(void)
{
}

static const RobotStateHandler state_handlers[ROBOT_STATE_COUNT] = {
    [ROBOT_STATE_A] = RobotStateA,
    [ROBOT_STATE_B] = RobotStateB,
};

void RobotStateMachineInit(void)
{
    current_state = ROBOT_STATE_A;
}

void RobotStateMachineTask(void)
{
    if (current_state < ROBOT_STATE_COUNT)
    {
        state_handlers[current_state]();
    }
}

void RobotStateMachineSetState(Robot_State_e state)
{
    if (state < ROBOT_STATE_COUNT)
    {
        current_state = state;
    }
}

Robot_State_e RobotStateMachineGetState(void)
{
    return current_state;
}
