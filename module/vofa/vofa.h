#ifndef VOFA_H
#define VOFA_H

#include <stdint.h>

#include "bsp_usart.h"

typedef union {
    float float_t;
    uint8_t uint8_t[4];
} send_float;

#define VOFA_JUSTFLOAT_MAX_NUM 17U

void vofa_justfloat_output(
    float *data, uint8_t num, UART_HandleTypeDef *huart);
Device_Status_e vofa_justfloat_output_dma(
    const float *data, uint8_t num, UART_HandleTypeDef *huart);

#endif
