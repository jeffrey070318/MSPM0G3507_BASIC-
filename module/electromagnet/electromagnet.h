#ifndef MODULE_ELECTROMAGNET_H
#define MODULE_ELECTROMAGNET_H

#include <stdbool.h>

#include "bsp_gpio.h"

typedef struct {
    GPIOInstance *gpio;
    GPIO_PinState active_state;
} Electromagnet_Device_t;

bool Electromagnet_Init(Electromagnet_Device_t *device,
    GPIO_TypeDef *gpio_port, uint32_t gpio_pin,
    GPIO_PinState active_state);
void Electromagnet_On(Electromagnet_Device_t *device);
void Electromagnet_Off(Electromagnet_Device_t *device);
void Electromagnet_Toggle(Electromagnet_Device_t *device);

#endif
