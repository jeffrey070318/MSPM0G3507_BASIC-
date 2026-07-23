#ifndef MOTOR_TEST_STUB_BSP_PWM_H
#define MOTOR_TEST_STUB_BSP_PWM_H

#include "bsp_device.h"

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    float period;
    float dutyratio;
    bool running;
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
void PWMStart(PWMInstance *instance);
void PWMStop(PWMInstance *instance);
void PWMSetDutyRatio(PWMInstance *instance, float dutyratio);

#endif
