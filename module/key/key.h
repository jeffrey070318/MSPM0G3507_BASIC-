#ifndef MODULE_KEY_H
#define MODULE_KEY_H

#include <stdbool.h>

#include "bsp_gpio.h"

typedef struct {
    GPIOInstance *gpio;
    GPIO_PinState active_state;
} KEY_Device_t;

bool KEY_Init(KEY_Device_t *key, GPIO_TypeDef *gpio_port,
    uint32_t gpio_pin, GPIO_PinState active_state);
bool KEY_IsPressed(KEY_Device_t *key);

#endif
