#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_device.h"

typedef struct {
    GPIO_TypeDef *phase_a_port;
    uint32_t phase_a_pin;
    GPIO_TypeDef *phase_b_port;
    uint32_t phase_b_pin;
    volatile bool reverse;
    volatile int32_t total_cnt;
    volatile int16_t speed;
    volatile uint32_t invalid_transition_count;
    int32_t last_sample_cnt;
    volatile uint8_t last_state;
    volatile bool started;
} Encoder_Device_t;

extern Encoder_Device_t hencoder_left;
extern Encoder_Device_t hencoder_right;

/** Initialize and reset the two SysConfig-owned encoder instances. */
void Encoder_BSP_Init(void);

/** Reset one encoder and synchronize its decoder with the current A/B state. */
void Encoder_Start(Encoder_Device_t *dev);

/** Process one GPIO edge. Intended to be called from GROUP1_IRQHandler. */
void Encoder_OnEdge(Encoder_Device_t *dev);

/** Snapshot the count change since the previous call into speed. */
void Encoder_Update(Encoder_Device_t *dev);

void Encoder_SetReverse(Encoder_Device_t *dev, bool reverse);
int32_t Encoder_Get_Total(const Encoder_Device_t *dev);
int16_t Encoder_Get_Speed(const Encoder_Device_t *dev);
uint32_t Encoder_Get_InvalidTransitions(const Encoder_Device_t *dev);

#endif
