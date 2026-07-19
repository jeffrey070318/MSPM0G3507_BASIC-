#include "bsp_encoder.h"

/* MSPM0 register-level counter access */
#define HAL_TIM_GET_COUNTER(htim)   ((htim)->Instance->COUNTERREGS.CTR)
#define HAL_TIM_SET_COUNTER(htim, v) ((htim)->Instance->COUNTERREGS.CTR = (v))

void Encoder_Start(Encoder_Device_t *dev)
{
    if (dev->htim == NULL)
        return;

    dev->total_cnt = 0;
    dev->speed     = 0;
    dev->last_cnt  = 0;
    HAL_TIM_SET_COUNTER(dev->htim, 0);
    DL_Timer_startCounter(dev->htim->Instance);
}

void Encoder_Update(Encoder_Device_t *dev)
{
    uint16_t current_cnt = (uint16_t)HAL_TIM_GET_COUNTER(dev->htim);
    int16_t  delta       = (int16_t)(current_cnt - dev->last_cnt);

    dev->speed     = delta;
    dev->total_cnt += delta;
    dev->last_cnt  = current_cnt;
}

int32_t Encoder_Get_Total(Encoder_Device_t *dev)
{
    return dev->total_cnt;
}

int16_t Encoder_Get_Speed(Encoder_Device_t *dev)
{
    return dev->speed;
}
