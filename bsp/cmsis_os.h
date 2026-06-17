#ifndef CMSIS_OS_H
#define CMSIS_OS_H

#include <stddef.h>
#include <stdint.h>

#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"

typedef TaskHandle_t osThreadId;

typedef enum {
    osPriorityIdle = tskIDLE_PRIORITY,
    osPriorityLow = tskIDLE_PRIORITY + 1,
    osPriorityNormal = tskIDLE_PRIORITY + 2,
    osPriorityHigh = tskIDLE_PRIORITY + 3,
    osPriorityRealtime = tskIDLE_PRIORITY + 4,
} osPriority;

typedef struct {
    const char *name;
    void (*pthread)(void const *argument);
    osPriority tpriority;
    uint32_t instances;
    uint32_t stacksize;
} osThreadDef_t;

#define osWaitForever portMAX_DELAY

static inline osThreadId osThreadCreate(
    const osThreadDef_t *thread_def, void *argument)
{
    TaskHandle_t handle = NULL;

    if (thread_def == NULL) {
        return NULL;
    }

    if (xTaskCreate((TaskFunction_t) thread_def->pthread, thread_def->name,
            (uint16_t) thread_def->stacksize, argument,
            (UBaseType_t) thread_def->tpriority, &handle) != pdPASS) {
        return NULL;
    }

    return handle;
}

static inline uint32_t osSignalSet(osThreadId thread_id, uint32_t signals)
{
    if (thread_id == NULL) {
        return 0U;
    }

    (void) xTaskNotify(thread_id, signals, eSetBits);
    return signals;
}

static inline uint32_t osSignalWait(uint32_t signals, uint32_t millisec)
{
    uint32_t value = 0U;
    (void) xTaskNotifyWait(0U, signals, &value, millisec);
    return value;
}

#else

typedef void *osThreadId;
typedef int osPriority;

typedef struct {
    const char *name;
    void (*pthread)(void const *argument);
    osPriority tpriority;
    uint32_t instances;
    uint32_t stacksize;
} osThreadDef_t;

#define osPriorityNormal (0)
#define osWaitForever (0xFFFFFFFFU)

static inline osThreadId osThreadCreate(
    const osThreadDef_t *thread_def, void *argument)
{
    (void) thread_def;
    (void) argument;
    return NULL;
}

static inline uint32_t osSignalSet(osThreadId thread_id, uint32_t signals)
{
    (void) thread_id;
    return signals;
}

static inline uint32_t osSignalWait(uint32_t signals, uint32_t millisec)
{
    (void) millisec;
    return signals;
}

#endif

#endif
