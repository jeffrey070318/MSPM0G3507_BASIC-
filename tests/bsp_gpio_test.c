#include <assert.h>
#include <stdint.h>

#include "bsp_gpio.h"

static uint32_t callback_a_count;
static uint32_t callback_b_count;

void DL_GPIO_togglePins(GPIO_TypeDef *gpio, uint32_t pins)
{
    gpio->output ^= pins;
    gpio->input = gpio->output;
}

void DL_GPIO_setPins(GPIO_TypeDef *gpio, uint32_t pins)
{
    gpio->output |= pins;
    gpio->input = gpio->output;
}

void DL_GPIO_clearPins(GPIO_TypeDef *gpio, uint32_t pins)
{
    gpio->output &= ~pins;
    gpio->input = gpio->output;
}

uint32_t DL_GPIO_readPins(GPIO_TypeDef *gpio, uint32_t pins)
{
    return gpio->input & pins;
}

static void CallbackA(GPIOInstance *instance)
{
    (void) instance;
    callback_a_count++;
}

static void CallbackB(GPIOInstance *instance)
{
    (void) instance;
    callback_b_count++;
}

int main(void)
{
    GPIO_TypeDef gpio_a = {0};
    GPIO_TypeDef gpio_b = {0};
    GPIO_Init_Config_s config_a = {
        .GPIOx = &gpio_a,
        .GPIO_Pin = 1UL << 5U,
        .gpio_model_callback = CallbackA,
    };
    GPIO_Init_Config_s config_b = {
        .GPIOx = &gpio_b,
        .GPIO_Pin = 1UL << 5U,
        .gpio_model_callback = CallbackB,
    };

    assert(GPIOPinFromInterruptIndex(DL_GPIO_IIDX_DIO0) == (1UL << 0U));
    assert(GPIOPinFromInterruptIndex(DL_GPIO_IIDX_DIO12) == (1UL << 12U));
    assert(GPIOPinFromInterruptIndex(DL_GPIO_IIDX_DIO31) == (1UL << 31U));
    assert(GPIOPinFromInterruptIndex(0U) == 0U);
    assert(GPIOPinFromInterruptIndex(33U) == 0U);

    GPIOInstance *instance_a = GPIORegister(&config_a);
    GPIOInstance *instance_b = GPIORegister(&config_b);
    assert(instance_a != NULL);
    assert(instance_b != NULL);

    GPIOInterruptCallbackForPort(&gpio_b, 1UL << 5U);
    assert(callback_a_count == 0U);
    assert(callback_b_count == 1U);

    assert(GPIORegister(&config_a) == NULL);
    GPIO_Init_Config_s invalid_config = config_a;
    invalid_config.GPIO_Pin = (1UL << 5U) | (1UL << 6U);
    assert(GPIORegister(&invalid_config) == NULL);

    GPIOSet(instance_a);
    assert(instance_a->pin_state == GPIO_PIN_SET);
    GPIOToggel(instance_a);
    assert(instance_a->pin_state == GPIO_PIN_RESET);

    return 0;
}
