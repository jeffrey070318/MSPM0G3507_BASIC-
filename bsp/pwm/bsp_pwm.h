#ifndef BSP_PWM_H
#define BSP_PWM_H

#include "bsp_device.h"

#define PWM_DEVICE_CNT 16U

typedef struct pwm_ins_temp {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    uint32_t tclk;
    float period;
    float dutyratio;
    bool running;
    void (*callback)(struct pwm_ins_temp *);
    void *id;
} PWMInstance;

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    float period;
    float dutyratio;
    void (*callback)(PWMInstance *);
    void *id;
} PWM_Init_Config_s;

PWMInstance *PWMRegister(PWM_Init_Config_s *config);
void PWMStart(PWMInstance *pwm);
void PWMSetDutyRatio(PWMInstance *pwm, float dutyratio);
void PWMStop(PWMInstance *pwm);
void PWMSetPeriod(PWMInstance *pwm, float period);
void PWMStartDMA(PWMInstance *pwm, uint32_t *pData, uint32_t Size);

#endif
