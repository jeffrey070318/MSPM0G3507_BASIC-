#include "bsp_encoder.h"

#include <limits.h>

static int16_t EncoderCalculateDelta(
    uint32_t current, uint32_t previous, uint32_t period)
{
    int64_t delta = (int64_t) current - (int64_t) previous;

    if (period > 1U) {
        int64_t half_period = (int64_t) period / 2;
        if (delta > half_period) {
            delta -= period;
        } else if (delta < -half_period) {
            delta += period;
        }
    }

    if (delta > INT16_MAX) {
        return INT16_MAX;
    }
    if (delta < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t) delta;
}

void Encoder_Start(Encoder_Device_t *dev)
{
    if (dev == NULL) {
        return;
    }

    dev->total_cnt = 0;
    dev->speed = 0;
    dev->last_cnt = 0U;

    if ((dev->htim != NULL) && (dev->htim->Instance != NULL)) {
        DL_Timer_startCounter(dev->htim->Instance);
    }
}

void Encoder_Update(Encoder_Device_t *dev)
{
    if ((dev == NULL) || (dev->htim == NULL) ||
        (dev->read_counter == NULL)) {
        return;
    }

    uint32_t current_cnt = dev->read_counter(dev->htim);
    int16_t delta = EncoderCalculateDelta(
        current_cnt, dev->last_cnt, dev->counter_period);

    dev->speed = delta;
    dev->total_cnt += delta;
    dev->last_cnt = current_cnt;
}

int32_t Encoder_Get_Total(Encoder_Device_t *dev)
{
    return (dev != NULL) ? dev->total_cnt : 0;
}

int16_t Encoder_Get_Speed(Encoder_Device_t *dev)
{
    return (dev != NULL) ? dev->speed : 0;
}
