#ifndef MODULE_SERVO_H
#define MODULE_SERVO_H

#include <stdbool.h>

#include "bsp_pwm.h"

#define SERVO_PERIOD_SECONDS (0.020f)
#define SERVO_MIN_ANGLE_DEGREES (0.0f)
#define SERVO_MAX_ANGLE_DEGREES (180.0f)

typedef struct {
    PWMInstance *pwm;
    float min_pulse_seconds;
    float max_pulse_seconds;
    float angle_degrees;
} SERVO_Device_t;

bool SERVO_Init(SERVO_Device_t *servo, TIM_HandleTypeDef *pwm_handle,
    float min_pulse_seconds, float max_pulse_seconds);
bool SERVO_SetAngle(SERVO_Device_t *servo, float angle_degrees);
void SERVO_Stop(SERVO_Device_t *servo);

#endif
