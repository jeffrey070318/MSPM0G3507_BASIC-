#include "bsp_gpio.h"
#include "bsp_memory.h"

#include <string.h>

static uint8_t idx;
static GPIOInstance *gpio_instance[GPIO_MX_DEVICE_NUM] = {NULL};

void GPIOInterruptCallback(uint32_t gpio_pin)
{
    for (uint8_t i = 0U; i < idx; ++i) {
        GPIOInstance *gpio = gpio_instance[i];
        if ((gpio != NULL) && (gpio->GPIO_Pin == gpio_pin) &&
            (gpio->gpio_model_callback != NULL)) {
            gpio->gpio_model_callback(gpio);
            return;
        }
    }
}

GPIOInstance *GPIORegister(GPIO_Init_Config_s *GPIO_config)
{
    if ((GPIO_config == NULL) || (GPIO_config->GPIOx == NULL) ||
        (GPIO_config->GPIO_Pin == 0U) || (idx >= GPIO_MX_DEVICE_NUM)) {
        return NULL;
    }

    GPIOInstance *instance =
        (GPIOInstance *) BSPMalloc(sizeof(GPIOInstance));
    if (instance == NULL) {
        return NULL;
    }
    memset(instance, 0, sizeof(GPIOInstance));

    instance->GPIOx = GPIO_config->GPIOx;
    instance->GPIO_Pin = GPIO_config->GPIO_Pin;
    instance->pin_state = GPIO_config->pin_state;
    instance->exti_mode = GPIO_config->exti_mode;
    instance->id = GPIO_config->id;
    instance->gpio_model_callback = GPIO_config->gpio_model_callback;
    gpio_instance[idx++] = instance;
    return instance;
}

void GPIOUnregister(GPIOInstance *instance)
{
    if (instance == NULL) {
        return;
    }

    for (uint8_t i = 0U; i < idx; i++) {
        if (gpio_instance[i] != instance) {
            continue;
        }

        for (uint8_t j = i; (j + 1U) < idx; j++) {
            gpio_instance[j] = gpio_instance[j + 1U];
        }
        idx--;
        gpio_instance[idx] = NULL;
        BSPFree(instance);
        return;
    }
}

void GPIOToggel(GPIOInstance *_instance)
{
    if ((_instance != NULL) && (_instance->GPIOx != NULL)) {
        DL_GPIO_togglePins(_instance->GPIOx, _instance->GPIO_Pin);
    }
}

void GPIOSet(GPIOInstance *_instance)
{
    if ((_instance != NULL) && (_instance->GPIOx != NULL)) {
        DL_GPIO_setPins(_instance->GPIOx, _instance->GPIO_Pin);
        _instance->pin_state = GPIO_PIN_SET;
    }
}

void GPIOReset(GPIOInstance *_instance)
{
    if ((_instance != NULL) && (_instance->GPIOx != NULL)) {
        DL_GPIO_clearPins(_instance->GPIOx, _instance->GPIO_Pin);
        _instance->pin_state = GPIO_PIN_RESET;
    }
}

GPIO_PinState GPIORead(GPIOInstance *_instance)
{
    if ((_instance == NULL) || (_instance->GPIOx == NULL)) {
        return GPIO_PIN_RESET;
    }

    _instance->pin_state =
        (DL_GPIO_readPins(_instance->GPIOx, _instance->GPIO_Pin) != 0U)
            ? GPIO_PIN_SET
            : GPIO_PIN_RESET;
    return _instance->pin_state;
}
