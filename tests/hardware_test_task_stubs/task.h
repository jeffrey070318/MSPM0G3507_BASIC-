#ifndef TEST_HARDWARE_TASK_TASK_H
#define TEST_HARDWARE_TASK_TASK_H

#include <stdint.h>

typedef int BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;
typedef uint32_t configSTACK_DEPTH_TYPE;
typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *argument);

#define pdPASS 1
#define tskIDLE_PRIORITY 0U
#define pdMS_TO_TICKS(milliseconds) ((TickType_t) (milliseconds))

BaseType_t xTaskCreate(TaskFunction_t task_code, const char *task_name,
    configSTACK_DEPTH_TYPE stack_depth, void *argument,
    UBaseType_t priority, TaskHandle_t *task_handle);
TickType_t xTaskGetTickCount(void);
void vTaskDelayUntil(TickType_t *last_wake_time, TickType_t period_ticks);

#endif
