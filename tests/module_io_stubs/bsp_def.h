#ifndef TEST_MODULE_IO_BSP_DEF_H
#define TEST_MODULE_IO_BSP_DEF_H

#include <stdint.h>

typedef enum {
    DEVICE_OK = 0,
    DEVICE_ERROR,
    DEVICE_BUSY,
    DEVICE_TIMEOUT,
} Device_Status_e;

#endif
