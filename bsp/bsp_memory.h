#ifndef BSP_MEMORY_H
#define BSP_MEMORY_H

#include <stddef.h>

#ifdef USE_FREERTOS
#include "FreeRTOS.h"

static inline void *BSPMalloc(size_t size)
{
    return pvPortMalloc(size);
}

static inline void BSPFree(void *memory)
{
    vPortFree(memory);
}
#else
#include <stdlib.h>

static inline void *BSPMalloc(size_t size)
{
    return malloc(size);
}

static inline void BSPFree(void *memory)
{
    free(memory);
}
#endif

#endif
