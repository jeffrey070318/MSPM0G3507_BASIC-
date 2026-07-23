#ifndef MODULE_LED_H
#define MODULE_LED_H

#include <stdbool.h>

#include "bsp_gpio.h"

typedef struct {
    GPIOInstance *gpio;
} LED_Device_t;

bool LED_Init(
    LED_Device_t *led, GPIO_TypeDef *gpio_port, uint32_t gpio_pin);
void LED_On(LED_Device_t *led);
void LED_Off(LED_Device_t *led);
void LED_Toggle(LED_Device_t *led);

#endif
