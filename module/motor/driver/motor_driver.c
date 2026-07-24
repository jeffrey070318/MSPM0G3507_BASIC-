#include "motor_driver.h"

#include <stddef.h>
#include <string.h>

#include "drv8701e_driver.h"
#include "tb6612_driver.h"

bool MotorDriver_Init(
    Motor_Driver_t *driver, const Motor_Driver_Init_Config_t *config)
{
    if ((driver == NULL) || (config == NULL)) {
        return false;
    }

    memset(driver, 0, sizeof(*driver));
    driver->type = config->type;

    bool initialized = false;
    switch (config->type) {
    case MOTOR_DRIVER_DRV8701E:
        initialized = DRV8701EDriver_Init(
            driver, &config->config.drv8701e);
        break;
    case MOTOR_DRIVER_TB6612:
        initialized = TB6612Driver_Init(
            driver, &config->config.tb6612);
        break;
    default:
        return false;
    }

    driver->initialized = initialized;
    return initialized;
}

void MotorDriver_SetOutput(Motor_Driver_t *driver, float output)
{
    if ((driver == NULL) || !driver->initialized ||
        (driver->ops == NULL) || (driver->ops->set_output == NULL)) {
        return;
    }
    driver->ops->set_output(driver, output);
}

void MotorDriver_Stop(Motor_Driver_t *driver)
{
    if ((driver == NULL) || !driver->initialized ||
        (driver->ops == NULL) || (driver->ops->stop == NULL)) {
        return;
    }
    driver->ops->stop(driver);
}
