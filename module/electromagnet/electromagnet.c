#include "electromagnet.h"

#include <stddef.h>

bool Electromagnet_Init(Electromagnet_Device_t *device,
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
    if (device->gpio == NULL) {
        return false;
    }

    Electromagnet_Off(device);
    return true;
}

void Electromagnet_On(Electromagnet_Device_t *device)
{
    if ((device == NULL) || (device->gpio == NULL)) {
        return;
    }

    if (device->active_state == GPIO_PIN_SET) {
        GPIOSet(device->gpio);
    } else {
        GPIOReset(device->gpio);
    }
}

void Electromagnet_Off(Electromagnet_Device_t *device)
{
    if ((device == NULL) || (device->gpio == NULL)) {
        return;
    }

    if (device->active_state == GPIO_PIN_SET) {
        GPIOReset(device->gpio);
    } else {
        GPIOSet(device->gpio);
    }
}

void Electromagnet_Toggle(Electromagnet_Device_t *device)
{
    if ((device != NULL) && (device->gpio != NULL)) {
        GPIOToggel(device->gpio);
    }
}
