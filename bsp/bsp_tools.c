#include "bsp_tools.h"

#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"

#define CALLBACK_TASK_CNT 32U

typedef struct {
    CallbackTaskFunction_t callback;
    uint32_t signal;
    void const *instance;
} CallbackTask_t;

static uint8_t callback_task_count;
static TaskHandle_t callback_task_handle[CALLBACK_TASK_CNT];
static CallbackTask_t callback_task_info[CALLBACK_TASK_CNT];

static void CallbackTaskBase(void *argument)
{
    CallbackTask_t *task = (CallbackTask_t *) argument;

    for (;;) {
        task->callback(task->instance);

        uint32_t notified_value = 0U;
        (void) xTaskNotifyWait(
            0U, task->signal, &notified_value, portMAX_DELAY);
    }
}
#endif

uint32_t CreateCallbackTask(const char *name, CallbackTaskFunction_t callback,
    void *instance, uint32_t priority)
{
#ifdef USE_FREERTOS
    if ((name == NULL) || (callback == NULL) ||
        (callback_task_count >= CALLBACK_TASK_CNT)) {
        return 0U;
    }

    uint8_t index = callback_task_count;
    CallbackTask_t *task = &callback_task_info[index];
    task->callback = callback;
    task->signal = 1UL << index;
    task->instance = instance;

    if (xTaskCreate(CallbackTaskBase, name, 128U, task,
            (UBaseType_t) priority, &callback_task_handle[index]) != pdPASS) {
        return 0U;
    }

    callback_task_count++;
    return task->signal;
#else
    (void) name;
    (void) callback;
    (void) instance;
    (void) priority;
    return 0U;
#endif
}
