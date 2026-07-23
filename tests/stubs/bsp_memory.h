#ifndef TEST_STUB_BSP_MEMORY_H
#define TEST_STUB_BSP_MEMORY_H

#include <stddef.h>
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
