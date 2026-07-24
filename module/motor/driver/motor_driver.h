#ifndef MODULE_MOTOR_DRIVER_H
#define MODULE_MOTOR_DRIVER_H

#include <stdbool.h>

#include "bsp_gpio.h"
#include "bsp_pwm.h"

typedef enum {
    MOTOR_DRIVER_DRV8701E = 0,
    MOTOR_DRIVER_TB6612,
} Motor_Driver_Type_e;

typedef enum {
    MOTOR_STOP_COAST = 0,
    MOTOR_STOP_BRAKE,
} Motor_Stop_Mode_e;

typedef struct {
    TIM_HandleTypeDef *pwm_handle;
    uint32_t pwm_channel;
    float pwm_period;
    GPIO_TypeDef *phase_port;
    uint32_t phase_pin;
    bool reverse;
} DRV8701E_Driver_Init_Config_t;

typedef struct {
    TIM_HandleTypeDef *pwm_handle;
    uint32_t pwm_channel;
    float pwm_period;
    GPIO_TypeDef *in1_port;
    uint32_t in1_pin;
    GPIO_TypeDef *in2_port;
    uint32_t in2_pin;
    bool reverse;
    Motor_Stop_Mode_e stop_mode;
} TB6612_Driver_Init_Config_t;

typedef struct {
    Motor_Driver_Type_e type;
    union {
        DRV8701E_Driver_Init_Config_t drv8701e;
        TB6612_Driver_Init_Config_t tb6612;
    } config;
} Motor_Driver_Init_Config_t;

typedef struct {
    PWMInstance *enable_pwm;
    GPIOInstance *phase;
    bool reverse;
    bool forward;
    bool direction_initialized;
} DRV8701E_Driver_t;

typedef struct {
    PWMInstance *pwm;
    GPIOInstance *in1;
    GPIOInstance *in2;
    bool reverse;
    Motor_Stop_Mode_e stop_mode;
} TB6612_Driver_t;

typedef struct Motor_Driver_s Motor_Driver_t;

typedef struct {
    void (*set_output)(Motor_Driver_t *driver, float output);
    void (*stop)(Motor_Driver_t *driver);
} Motor_Driver_Ops_t;

struct Motor_Driver_s {
    const Motor_Driver_Ops_t *ops;
    Motor_Driver_Type_e type;
    union {
        DRV8701E_Driver_t drv8701e;
        TB6612_Driver_t tb6612;
    } context;
    bool initialized;
};

bool MotorDriver_Init(
    Motor_Driver_t *driver, const Motor_Driver_Init_Config_t *config);
void MotorDriver_SetOutput(Motor_Driver_t *driver, float output);
void MotorDriver_Stop(Motor_Driver_t *driver);

#endif
