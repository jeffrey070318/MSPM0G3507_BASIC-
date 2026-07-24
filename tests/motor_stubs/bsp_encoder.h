#ifndef MOTOR_TEST_STUB_BSP_ENCODER_H
#define MOTOR_TEST_STUB_BSP_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t speed;
    bool reverse;
    bool started;
    uint32_t start_count;
    uint32_t update_count;
} Encoder_Device_t;

void Encoder_Start(Encoder_Device_t *encoder);
void Encoder_Update(Encoder_Device_t *encoder);
void Encoder_SetReverse(Encoder_Device_t *encoder, bool reverse);
int16_t Encoder_Get_Speed(const Encoder_Device_t *encoder);

#endif
