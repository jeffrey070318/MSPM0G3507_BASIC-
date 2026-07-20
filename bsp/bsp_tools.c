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

static int32_t CallbackTaskFindIndex(uint32_t signal)
{
    if ((signal == 0U) || ((signal & (signal - 1U)) != 0U)) {
        return -1;
    }

    for (uint8_t i = 0U; i < callback_task_count; ++i) {
        if (callback_task_info[i].signal == signal) {
            return (int32_t) i;
        }
    }
    return -1;
}

static void CallbackTaskBase(void *argument)
{
    CallbackTask_t *task = (CallbackTask_t *) argument;

    for (;;) {
        uint32_t notified_value = 0U;
        if ((xTaskNotifyWait(0U, task->signal, &notified_value,
                 portMAX_DELAY) == pdTRUE) &&
            ((notified_value & task->signal) != 0U)) {
            task->callback(task->instance);
        }
    }
}
#endif

uint32_t CreateCallbackTask(const char *name, CallbackTaskFunction_t callback,
    void *instance, uint32_t priority)
{
#ifdef USE_FREERTOS
    if ((name == NULL) || (callback == NULL) ||
        (callback_task_count >= CALLBACK_TASK_CNT) ||
        (priority >= configMAX_PRIORITIES)) {
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

uint8_t NotifyCallbackTask(uint32_t signal)
{
#ifdef USE_FREERTOS
    int32_t index = CallbackTaskFindIndex(signal);
    if (index < 0) {
        return 0U;
    }

    return (xTaskNotify(callback_task_handle[index], signal, eSetBits) == pdPASS)
        ? 1U
        : 0U;
#else
    (void) signal;
    return 0U;
#endif
}

uint8_t NotifyCallbackTaskFromISR(uint32_t signal)
{
#ifdef USE_FREERTOS
    int32_t index = CallbackTaskFindIndex(signal);
    if (index < 0) {
        return 0U;
    }

    BaseType_t higher_priority_task_woken = pdFALSE;
    BaseType_t status = xTaskNotifyFromISR(callback_task_handle[index], signal,
        eSetBits, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return (status == pdPASS) ? 1U : 0U;
#else
    (void) signal;
    return 0U;
#endif
}
