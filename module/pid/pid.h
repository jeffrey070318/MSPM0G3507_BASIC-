#ifndef PID_H
#define PID_H

typedef enum {
    PID_POSITION = 0,
    PID_DELTA,
} PID_Mode_e;

typedef struct {
    PID_Mode_e mode;
    float Kp;
    float Ki;
    float Kd;
    float max_out;
    float max_iout;
    float set;
    float fdb;
    float out;
    float Pout;
    float Iout;
    float Dout;
    float error[3];
} PID_Device_t;

void PID_Init(PID_Device_t *pid, PID_Mode_e mode, float kp, float ki,
    float kd, float max_out, float max_iout);
float PID_Calc(PID_Device_t *pid, float fdb, float set);
void PID_Clear(PID_Device_t *pid);
void PID_SetParam(PID_Device_t *pid, float kp, float ki, float kd);

#endif
