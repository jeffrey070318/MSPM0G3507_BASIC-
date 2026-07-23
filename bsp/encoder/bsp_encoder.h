#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include <stdint.h>
#include <stdbool.h>

#include "bsp_device.h"

#define ENCODER_MAX_DEVICES 2U

typedef struct {
    GPIO_TypeDef    *port_a;
    uint32_t         pin_a;
    GPIO_TypeDef    *port_b;
    uint32_t         pin_b;
    volatile int32_t total_cnt;
    volatile int16_t speed;
    int32_t          last_total;
} Encoder_Device_t;

void Encoder_Init(Encoder_Device_t *dev);
void Encoder_Update(Encoder_Device_t *dev);
int32_t Encoder_Get_Total(Encoder_Device_t *dev);
int16_t Encoder_Get_Speed(Encoder_Device_t *dev);

void Encoder_ISR_ByPortPin(GPIO_TypeDef *port, uint32_t pin);

#endif