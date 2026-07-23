#ifndef VOFA_H
#define VOFA_H

#include <stdint.h>

#include "bsp_def.h"

typedef union {
    float float_t;
    uint8_t uint8_t[4];
} send_float;

#define VOFA_JUSTFLOAT_MAX_NUM 17U

Device_Status_e VOFA_Init(void);
Device_Status_e VOFA_JustFloatOutput(const float *data, uint8_t num);
Device_Status_e VOFA_JustFloatOutputDMA(const float *data, uint8_t num);

#endif
