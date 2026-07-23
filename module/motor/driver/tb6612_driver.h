#ifndef MODULE_MOTOR_TB6612_DRIVER_H
#define MODULE_MOTOR_TB6612_DRIVER_H

#include "motor_driver.h"

bool TB6612Driver_Init(Motor_Driver_t *driver,
    const TB6612_Driver_Init_Config_t *config);

#endif
