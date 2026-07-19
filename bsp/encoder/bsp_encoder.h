#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include "bsp_device.h"
#include "stdint.h"

typedef struct
{
    TIM_HandleTypeDef *htim;
    int32_t            total_cnt;
    int16_t            speed;
    uint16_t           last_cnt;
} Encoder_Device_t;

void    Encoder_Start(Encoder_Device_t *dev);
void    Encoder_Update(Encoder_Device_t *dev);
int32_t Encoder_Get_Total(Encoder_Device_t *dev);
int16_t Encoder_Get_Speed(Encoder_Device_t *dev);

#endif

