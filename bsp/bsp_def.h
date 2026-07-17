#ifndef BSP_DEF_H
#define BSP_DEF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

typedef enum {
    DEVICE_OK = 0,
    DEVICE_ERROR,
    DEVICE_BUSY,
    DEVICE_TIMEOUT,
} Device_Status_e;

#endif
