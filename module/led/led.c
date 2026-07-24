#include "led.h"

#include <stddef.h>

bool LED_Init(
    LED_Device_t *led, GPIO_TypeDef *gpio_port, uint32_t gpio_pin)
{
    if ((led == NULL) || (gpio_port == NULL) || (gpio_pin == 0U)) {
        return false;
    }

    led->gpio = NULL;
    GPIO_Init_Config_s config = {
        .GPIOx = gpio_port,
        .GPIO_Pin = gpio_pin,
        .pin_state = GPIO_PIN_RESET,
        .exti_mode = GPIO_EXTI_MODE_NONE,
        .gpio_model_callback = NULL,
        .id = led,
    };
    led->gpio = GPIORegister(&config);
    if (led->gpio == NULL) {
        return false;
    }

    LED_Off(led);
    return true;
}

void LED_On(LED_Device_t *led)
{
    if ((led != NULL) && (led->gpio != NULL)) {
        GPIOSet(led->gpio);
    }
}

void LED_Off(LED_Device_t *led)
{
    if ((led != NULL) && (led->gpio != NULL)) {
        GPIOReset(led->gpio);
    }
}

void LED_Toggle(LED_Device_t *led)
{
    if ((led != NULL) && (led->gpio != NULL)) {
        GPIOToggel(led->gpio);
    }
}
