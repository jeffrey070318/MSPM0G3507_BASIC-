#ifndef TEST_CHASSIS_MOTOR_H
#define TEST_CHASSIS_MOTOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t Channel;
} TIM_HandleTypeDef;

typedef struct {
    uint32_t marker;
} GPIO_TypeDef;

typedef struct {
    uint32_t marker;
} Encoder_Device_t;

typedef struct {
    float kp;
    float ki;
    float kd;
    float output_limit;
    float integral_limit;
    float deadband;
    bool derivative_on_measurement;
} PID_Config_t;

typedef enum {
    MOTOR_DRIVER_DRV8701E = 0,
    MOTOR_DRIVER_TB6612,
} Motor_Driver_Type_e;

typedef struct {
    TIM_HandleTypeDef *pwm_handle;
    uint32_t pwm_channel;
    float pwm_period;
    GPIO_TypeDef *phase_port;
    uint32_t phase_pin;
    bool reverse;
} DRV8701E_Driver_Init_Config_t;

typedef struct {
    Motor_Driver_Type_e type;
    union {
        DRV8701E_Driver_Init_Config_t drv8701e;
    } config;
} Motor_Driver_Init_Config_t;

typedef struct {
    Motor_Driver_Init_Config_t driver;
    Encoder_Device_t *encoder;
    PID_Config_t speed_pid;
    bool encoder_reverse;
} Motor_Init_Config_t;

typedef struct {
    Encoder_Device_t *encoder;
    float target_speed;
    float measured_speed;
    float control_output;
    bool enabled;
} Motor_Device_t;

bool Motor_Init(Motor_Device_t *motor, const Motor_Init_Config_t *config);
void Motor_SetTargetSpeed(Motor_Device_t *motor, float counts_per_second);
bool Motor_Update(Motor_Device_t *motor, float dt_seconds);
void Motor_Enable(Motor_Device_t *motor);
void Motor_Stop(Motor_Device_t *motor);

#endif
