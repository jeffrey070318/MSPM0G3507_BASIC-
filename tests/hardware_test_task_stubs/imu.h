#ifndef TEST_HARDWARE_TASK_IMU_H
#define TEST_HARDWARE_TASK_IMU_H

#include "bsp_def.h"

typedef struct {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float roll;
    float pitch;
    float yaw;
} IMU_Data_t;

Device_Status_e IMU_Init(void);
Device_Status_e IMU_ReadAll(IMU_Data_t *data);

#endif
