#include "pid.h"

#include <stddef.h>

static float PIDAbsolute(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float PIDLimit(float value, float limit)
{
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

bool PID_ControllerInit(
    PID_Controller_t *pid, const PID_Config_t *config)
{
    if ((pid == NULL) || (config == NULL) ||
        !(config->output_limit >= 0.0f) ||
        !(config->integral_limit >= 0.0f) ||
        !(config->deadband >= 0.0f)) {
        return false;
    }

    pid->config = *config;
    pid->initialized = true;
    PID_ControllerReset(pid);
    return true;
}

float PID_ControllerUpdate(PID_Controller_t *pid, float setpoint,
    float measurement, float dt_seconds)
{
    if ((pid == NULL) || !pid->initialized || !(dt_seconds > 0.0f)) {
        return 0.0f;
    }

    float error = setpoint - measurement;
    if (PIDAbsolute(error) <= pid->config.deadband) {
        error = 0.0f;
    }

    float derivative = 0.0f;
    if (pid->has_previous_sample) {
        if (pid->config.derivative_on_measurement) {
            derivative = -(measurement - pid->last_measurement) / dt_seconds;
        } else {
            derivative = (error - pid->last_error) / dt_seconds;
        }
    }

    float proportional = pid->config.kp * error;
    float integral_step = pid->config.ki * error * dt_seconds;
    float candidate_integral = PIDLimit(
        pid->integral + integral_step, pid->config.integral_limit);
    float derivative_output = pid->config.kd * derivative;
    float candidate_output =
        proportional + candidate_integral + derivative_output;

    if (((candidate_output > pid->config.output_limit) &&
         (integral_step > 0.0f)) ||
        ((candidate_output < -pid->config.output_limit) &&
         (integral_step < 0.0f))) {
        candidate_integral = pid->integral;
        candidate_output = proportional + candidate_integral + derivative_output;
    }

    pid->setpoint = setpoint;
    pid->measurement = measurement;
    pid->error = error;
    pid->integral = candidate_integral;
    pid->derivative = derivative;
    pid->output = PIDLimit(candidate_output, pid->config.output_limit);
    pid->last_error = error;
    pid->last_measurement = measurement;
    pid->has_previous_sample = true;

    return pid->output;
}

void PID_ControllerReset(PID_Controller_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->setpoint = 0.0f;
    pid->measurement = 0.0f;
    pid->error = 0.0f;
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->output = 0.0f;
    pid->last_error = 0.0f;
    pid->last_measurement = 0.0f;
    pid->has_previous_sample = false;
}

void PID_ControllerSetTunings(
    PID_Controller_t *pid, float kp, float ki, float kd)
{
    if (pid == NULL) {
        return;
    }

    pid->config.kp = kp;
    pid->config.ki = ki;
    pid->config.kd = kd;
}
