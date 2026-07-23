#include "bsp_encoder.h"

#include <limits.h>

#include "bsp_encoder_decode.h"

Encoder_Device_t hencoder_left = {
    .phase_a_port = ENCODER_GPIO_ENC_L_A_PORT,
    .phase_a_pin = ENCODER_GPIO_ENC_L_A_PIN,
    .phase_b_port = ENCODER_GPIO_ENC_L_B_PORT,
    .phase_b_pin = ENCODER_GPIO_ENC_L_B_PIN,
    .reverse = false,
};

Encoder_Device_t hencoder_right = {
    .phase_a_port = ENCODER_GPIO_ENC_R_A_PORT,
    .phase_a_pin = ENCODER_GPIO_ENC_R_A_PIN,
    .phase_b_port = ENCODER_GPIO_ENC_R_B_PORT,
    .phase_b_pin = ENCODER_GPIO_ENC_R_B_PIN,
    .reverse = false,
};

static uint8_t EncoderReadState(const Encoder_Device_t *dev)
{
    uint8_t phase_a =
        (DL_GPIO_readPins(dev->phase_a_port, dev->phase_a_pin) != 0U) ? 1U : 0U;
    uint8_t phase_b =
        (DL_GPIO_readPins(dev->phase_b_port, dev->phase_b_pin) != 0U) ? 1U : 0U;
    return (uint8_t) ((phase_a << 1U) | phase_b);
}

static int16_t EncoderClampSpeed(int64_t delta)
{
    if (delta > INT16_MAX) {
        return INT16_MAX;
    }
    if (delta < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t) delta;
}

static void EncoderAccumulate(Encoder_Device_t *dev, int8_t delta)
{
    if ((delta > 0) && (dev->total_cnt < INT32_MAX)) {
        dev->total_cnt++;
    } else if ((delta < 0) && (dev->total_cnt > INT32_MIN)) {
        dev->total_cnt--;
    }
}

void Encoder_BSP_Init(void)
{
    Encoder_Start(&hencoder_left);
    Encoder_Start(&hencoder_right);
}

void Encoder_Start(Encoder_Device_t *dev)
{
    if ((dev == NULL) || (dev->phase_a_port == NULL) ||
        (dev->phase_b_port == NULL) || (dev->phase_a_pin == 0U) ||
        (dev->phase_b_pin == 0U)) {
        return;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    dev->total_cnt = 0;
    dev->speed = 0;
    dev->invalid_transition_count = 0U;
    dev->last_sample_cnt = 0;
    dev->last_state = EncoderReadState(dev);
    dev->started = true;
    if (primask == 0U) {
        __enable_irq();
    }
}

void Encoder_OnEdge(Encoder_Device_t *dev)
{
    if ((dev == NULL) || !dev->started) {
        return;
    }

    uint8_t current_state = EncoderReadState(dev);
    int8_t delta = EncoderDecodeTransition(dev->last_state, current_state);
    if ((delta == 0) && (current_state != dev->last_state)) {
        dev->invalid_transition_count++;
    }
    dev->last_state = current_state;

    if (dev->reverse) {
        delta = (int8_t) -delta;
    }
    EncoderAccumulate(dev, delta);
}

void Encoder_Update(Encoder_Device_t *dev)
{
    if ((dev == NULL) || !dev->started) {
        return;
    }

    int32_t current = dev->total_cnt;
    int64_t delta = (int64_t) current - (int64_t) dev->last_sample_cnt;
    dev->speed = EncoderClampSpeed(delta);
    dev->last_sample_cnt = current;
}

void Encoder_SetReverse(Encoder_Device_t *dev, bool reverse)
{
    if (dev != NULL) {
        dev->reverse = reverse;
    }
}

int32_t Encoder_Get_Total(const Encoder_Device_t *dev)
{
    return (dev != NULL) ? dev->total_cnt : 0;
}

int16_t Encoder_Get_Speed(const Encoder_Device_t *dev)
{
    return (dev != NULL) ? dev->speed : 0;
}

uint32_t Encoder_Get_InvalidTransitions(const Encoder_Device_t *dev)
{
    return (dev != NULL) ? dev->invalid_transition_count : 0U;
}
