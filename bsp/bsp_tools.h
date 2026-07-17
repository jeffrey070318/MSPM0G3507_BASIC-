#ifndef BSP_TOOLS_H
#define BSP_TOOLS_H

#include <stdint.h>

typedef void (*CallbackTaskFunction_t)(void const *instance);

uint32_t CreateCallbackTask(const char *name, CallbackTaskFunction_t callback,
    void *instance, uint32_t priority);

#endif
