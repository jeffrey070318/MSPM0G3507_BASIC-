#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "robot_task.h"

static const char *created_task_names[4];
static unsigned created_task_count;

volatile FrameworkBootStage_e framework_boot_stage;
volatile uint32_t framework_robot_heartbeat;
volatile uint32_t framework_main_heartbeat;

BaseType_t xTaskCreate(TaskFunction_t task_code, const char *task_name,
    configSTACK_DEPTH_TYPE stack_depth, void *argument,
    UBaseType_t priority, TaskHandle_t *task_handle)
{
    (void) stack_depth;
    (void) argument;
    (void) priority;
    assert(created_task_count < 4U);
    created_task_names[created_task_count++] = task_name;
    if (task_handle != NULL) {
        *task_handle = (TaskHandle_t) task_code;
    }
    return pdPASS;
}

TickType_t xTaskGetTickCount(void)
{
    return 0U;
}

void vTaskDelayUntil(TickType_t *last_wake_time, TickType_t period_ticks)
{
    (void) last_wake_time;
    (void) period_ticks;
}

void RobotTask(void)
{
}

void StartHARDWARETESTTASK(void *argument)
{
    (void) argument;
}

int main(void)
{
    OSTaskInit();
    assert(created_task_count == 1U);
    assert(strcmp(created_task_names[0], "hardwaretest") == 0);
    return 0;
}
