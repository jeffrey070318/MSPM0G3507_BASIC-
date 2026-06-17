#include "FreeRTOS.h"
#include "task.h"

#if (configSUPPORT_STATIC_ALLOCATION == 1)
void vApplicationGetIdleTaskMemory(StaticTask_t **idle_task_tcb,
                                   StackType_t **idle_task_stack,
                                   configSTACK_DEPTH_TYPE *idle_task_stack_size)
{
    static StaticTask_t idle_tcb;
    static StackType_t idle_stack[configIDLE_TASK_STACK_DEPTH];

    *idle_task_tcb = &idle_tcb;
    *idle_task_stack = idle_stack;
    *idle_task_stack_size = configIDLE_TASK_STACK_DEPTH;
}

#if (configUSE_TIMERS == 1)
void vApplicationGetTimerTaskMemory(StaticTask_t **timer_task_tcb,
                                    StackType_t **timer_task_stack,
                                    configSTACK_DEPTH_TYPE *timer_task_stack_size)
{
    static StaticTask_t timer_tcb;
    static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];

    *timer_task_tcb = &timer_tcb;
    *timer_task_stack = timer_stack;
    *timer_task_stack_size = configTIMER_TASK_STACK_DEPTH;
}
#endif
#endif
