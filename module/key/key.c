#include "key.h"

#include <stddef.h>

bool KEY_Init(KEY_Device_t *key, GPIO_TypeDef *gpio_port,
    uint32_t gpio_pin, GPIO_PinState active_state)
{
    if ((key == NULL) || (gpio_port == NULL) || (gpio_pin == 0U) ||
        (active_state > GPIO_PIN_SET)) {
        return false;
    }

    key->gpio = NULL;
    key->active_state = active_state;
    GPIO_Init_Config_s config = {
        .GPIOx = gpio_port,
        .GPIO_Pin = gpio_pin,
        .pin_state = GPIO_PIN_RESET,
        .exti_mode = GPIO_EXTI_MODE_NONE,
        .gpio_model_callback = NULL,
        .id = key,
    };
    key->gpio = GPIORegister(&config);
    return key->gpio != NULL;
}

bool KEY_IsPressed(KEY_Device_t *key)
{
    return (key != NULL) && (key->gpio != NULL) &&
           (GPIORead(key->gpio) == key->active_state);
}
