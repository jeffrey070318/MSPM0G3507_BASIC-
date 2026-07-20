#include "pid.h"

#include <stddef.h>

static float PIDAbsolute(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float PIDLimit(float value, float limit)
{
    limit = PIDAbsolute(limit);
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

void PID_Init(PID_Device_t *pid, PID_Mode_e mode, float kp, float ki,
    float kd, float max_out, float max_iout)
{
    if (pid == NULL) {
        return;
    }

    pid->mode = mode;
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->max_out = PIDAbsolute(max_out);
    pid->max_iout = PIDAbsolute(max_iout);
    PID_Clear(pid);
}

void PID_Clear(PID_Device_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->set = 0.0f;
    pid->fdb = 0.0f;
    pid->out = 0.0f;
    pid->Pout = 0.0f;
    pid->Iout = 0.0f;
    pid->Dout = 0.0f;
    pid->error[0] = 0.0f;
    pid->error[1] = 0.0f;
    pid->error[2] = 0.0f;
}

float PID_Calc(PID_Device_t *pid, float fdb, float set)
{
    if (pid == NULL) {
        return 0.0f;
    }

    pid->set = set;
    pid->fdb = fdb;
    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->error[0] = set - fdb;

    if (pid->mode == PID_POSITION) {
        pid->Pout = pid->Kp * pid->error[0];
        pid->Iout += pid->Ki * pid->error[0];
        pid->Dout = pid->Kd * (pid->error[0] - pid->error[1]);
        pid->Iout = PIDLimit(pid->Iout, pid->max_iout);
        pid->out = pid->Pout + pid->Iout + pid->Dout;
    } else if (pid->mode == PID_DELTA) {
        pid->Pout = pid->Kp * (pid->error[0] - pid->error[1]);
        pid->Iout = pid->Ki * pid->error[0];
        pid->Dout = pid->Kd *
            (pid->error[0] - 2.0f * pid->error[1] + pid->error[2]);
        pid->out += pid->Pout + pid->Iout + pid->Dout;
    }

    pid->out = PIDLimit(pid->out, pid->max_out);
    return pid->out;
}

void PID_SetParam(PID_Device_t *pid, float kp, float ki, float kd)
{
    if (pid == NULL) {
        return;
    }

    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
}
