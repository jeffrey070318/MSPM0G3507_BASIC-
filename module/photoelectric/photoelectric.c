#include "photoelectric.h"

#include <stddef.h>

bool Photoelectric_Init(Photoelectric_Device_t *device,
    GPIO_TypeDef *gpio_port, uint32_t gpio_pin,
    GPIO_PinState active_state)
{
    if ((device == NULL) || (gpio_port == NULL) || (gpio_pin == 0U) ||
        (active_state > GPIO_PIN_SET)) {
        return false;
    }

    device->gpio = NULL;
    device->active_state = active_state;
    GPIO_Init_Config_s config = {
        .GPIOx = gpio_port,
        .GPIO_Pin = gpio_pin,
        .pin_state = GPIO_PIN_RESET,
        .exti_mode = GPIO_EXTI_MODE_NONE,
        .gpio_model_callback = NULL,
        .id = device,
    };
    device->gpio = GPIORegister(&config);
    return device->gpio != NULL;
}

bool Photoelectric_IsTriggered(Photoelectric_Device_t *device)
{
    return (device != NULL) && (device->gpio != NULL) &&
           (GPIORead(device->gpio) == device->active_state);
}
