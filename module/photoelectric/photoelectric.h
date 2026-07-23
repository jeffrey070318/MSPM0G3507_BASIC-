#ifndef MODULE_PHOTOELECTRIC_H
#define MODULE_PHOTOELECTRIC_H

#include <stdbool.h>

#include "bsp_gpio.h"

typedef struct {
    GPIOInstance *gpio;
    GPIO_PinState active_state;
} Photoelectric_Device_t;

bool Photoelectric_Init(Photoelectric_Device_t *device,
    GPIO_TypeDef *gpio_port, uint32_t gpio_pin,
    GPIO_PinState active_state);
bool Photoelectric_IsTriggered(Photoelectric_Device_t *device);

#endif
