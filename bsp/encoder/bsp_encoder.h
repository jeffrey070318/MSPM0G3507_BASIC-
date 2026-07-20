#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include <stdint.h>

#include "bsp_device.h"

typedef uint32_t (*Encoder_ReadCounter_f)(TIM_HandleTypeDef *htim);

typedef struct {
    TIM_HandleTypeDef *htim;
    Encoder_ReadCounter_f read_counter;
    uint32_t counter_period;
    int32_t total_cnt;
    int16_t speed;
    uint32_t last_cnt;
} Encoder_Device_t;

void Encoder_Start(Encoder_Device_t *dev);
void Encoder_Update(Encoder_Device_t *dev);
int32_t Encoder_Get_Total(Encoder_Device_t *dev);
int16_t Encoder_Get_Speed(Encoder_Device_t *dev);

#endif
