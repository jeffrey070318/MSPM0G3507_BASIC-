#ifndef BSP_TOOLS_H
#define BSP_TOOLS_H

#include <stdint.h>

typedef void (*CallbackTaskFunction_t)(void const *instance);

uint32_t CreateCallbackTask(const char *name, CallbackTaskFunction_t callback,
    void const *instance, uint32_t priority);
uint8_t NotifyCallbackTask(uint32_t signal);
uint8_t NotifyCallbackTaskFromISR(uint32_t signal);

#endif
