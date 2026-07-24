#ifndef MODULE_MOTOR_DRV8701E_DRIVER_H
#define MODULE_MOTOR_DRV8701E_DRIVER_H

#include "motor_driver.h"

bool DRV8701EDriver_Init(Motor_Driver_t *driver,
    const DRV8701E_Driver_Init_Config_t *config);

#endif
