#include "bsp_gpio.h"
#include "bsp_memory.h"

#include <string.h>

static volatile uint8_t idx;
static GPIOInstance * volatile gpio_instance[GPIO_MX_DEVICE_NUM] = {NULL};

static uint32_t GPIOEnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void GPIOExitCritical(uint32_t primask)
{
    if (primask == 0U) {
        __enable_irq();
    }
}

uint32_t GPIOPinFromInterruptIndex(uint32_t interrupt_index)
{
    if ((interrupt_index < (uint32_t) DL_GPIO_IIDX_DIO0) ||
        (interrupt_index > (uint32_t) DL_GPIO_IIDX_DIO31)) {
        return 0U;
    }
    return 1UL << (interrupt_index - (uint32_t) DL_GPIO_IIDX_DIO0);
}

void GPIOInterruptCallbackForPort(
    GPIO_TypeDef *gpio_port, uint32_t gpio_pin)
{
    uint8_t count = idx;
    for (uint8_t i = 0U; i < count; ++i) {
        GPIOInstance *gpio = gpio_instance[i];
        if ((gpio != NULL) && (gpio->GPIOx == gpio_port) &&
            (gpio->GPIO_Pin == gpio_pin) &&
            (gpio->gpio_model_callback != NULL)) {
            gpio->gpio_model_callback(gpio);
            return;
        }
    }
}

void GPIOInterruptCallback(uint32_t gpio_pin)
{
    uint8_t count = idx;
    for (uint8_t i = 0U; i < count; ++i) {
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
        (GPIO_config->GPIO_Pin == 0U) ||
        ((GPIO_config->GPIO_Pin & (GPIO_config->GPIO_Pin - 1U)) != 0U)) {
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

    uint32_t primask = GPIOEnterCritical();
    if (idx >= GPIO_MX_DEVICE_NUM) {
        GPIOExitCritical(primask);
        BSPFree(instance);
        return NULL;
    }
    for (uint8_t i = 0U; i < idx; ++i) {
        if ((gpio_instance[i] != NULL) &&
            (gpio_instance[i]->GPIOx == instance->GPIOx) &&
            (gpio_instance[i]->GPIO_Pin == instance->GPIO_Pin)) {
            GPIOExitCritical(primask);
            BSPFree(instance);
            return NULL;
        }
    }
    gpio_instance[idx++] = instance;
    GPIOExitCritical(primask);
    return instance;
}

void GPIOToggel(GPIOInstance *_instance)
{
    if ((_instance != NULL) && (_instance->GPIOx != NULL)) {
        DL_GPIO_togglePins(_instance->GPIOx, _instance->GPIO_Pin);
        _instance->pin_state =
            (DL_GPIO_readPins(_instance->GPIOx, _instance->GPIO_Pin) != 0U)
                ? GPIO_PIN_SET
                : GPIO_PIN_RESET;
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
