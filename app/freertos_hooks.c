#include "FreeRTOS.h"
#include "task.h"

enum {
    FREERTOS_FAULT_NONE = 0U,
    FREERTOS_FAULT_MALLOC_FAILED = 1U,
    FREERTOS_FAULT_STACK_OVERFLOW = 2U,
};

volatile uint32_t g_freertos_fault_code = FREERTOS_FAULT_NONE;
volatile TaskHandle_t g_freertos_fault_task = NULL;
const char * volatile g_freertos_fault_task_name = NULL;

static void FreeRTOSFaultStop(
    uint32_t fault_code, TaskHandle_t task, const char *task_name)
{
    taskDISABLE_INTERRUPTS();
    g_freertos_fault_code = fault_code;
    g_freertos_fault_task = task;
    g_freertos_fault_task_name = task_name;

    for (;;) {
    }
}

#if (configUSE_MALLOC_FAILED_HOOK == 1)
void vApplicationMallocFailedHook(void)
{
    FreeRTOSFaultStop(FREERTOS_FAULT_MALLOC_FAILED, NULL, NULL);
}
#endif

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    FreeRTOSFaultStop(FREERTOS_FAULT_STACK_OVERFLOW, task, task_name);
}
#endif

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
