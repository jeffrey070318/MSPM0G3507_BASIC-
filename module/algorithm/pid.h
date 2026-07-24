#ifndef MODULE_ALGORITHM_PID_H
#define MODULE_ALGORITHM_PID_H

#include <stdbool.h>

typedef struct {
    float kp;
    float ki;
    float kd;
    float output_limit;
    float integral_limit;
    float deadband;
    bool derivative_on_measurement;
} PID_Config_t;

typedef struct {
    PID_Config_t config;
    float setpoint;
    float measurement;
    float error;
    float integral;
    float derivative;
    float output;
    float last_error;
    float last_measurement;
    bool has_previous_sample;
    bool initialized;
} PID_Controller_t;

bool PID_ControllerInit(
    PID_Controller_t *pid, const PID_Config_t *config);
float PID_ControllerUpdate(PID_Controller_t *pid, float setpoint,
    float measurement, float dt_seconds);
void PID_ControllerReset(PID_Controller_t *pid);
void PID_ControllerSetTunings(
    PID_Controller_t *pid, float kp, float ki, float kd);

#endif
