#ifndef TEST_HARDWARE_TASK_VOFA_H
#define TEST_HARDWARE_TASK_VOFA_H

#include <stdint.h>

#include "bsp_def.h"

Device_Status_e VOFA_Init(void);
Device_Status_e VOFA_JustFloatOutputDMA(
    const float *channels, uint8_t channel_count);

#endif
