#include <assert.h>
#include <stddef.h>

#include "electromagnet.h"

static GPIOInstance gpio_instances[2];
static uint32_t gpio_instance_count;
static bool fail_registration;

GPIOInstance *GPIORegister(GPIO_Init_Config_s *config)
{
    if (fail_registration || (config == NULL) ||
        (gpio_instance_count >= 2U)) {
        return NULL;
    }

    GPIOInstance *instance = &gpio_instances[gpio_instance_count++];
    instance->GPIOx = config->GPIOx;
    instance->GPIO_Pin = config->GPIO_Pin;
    instance->pin_state = config->pin_state;
    return instance;
}

void GPIOSet(GPIOInstance *instance)
{
    instance->GPIOx->output |= instance->GPIO_Pin;
    instance->pin_state = GPIO_PIN_SET;
}

void GPIOReset(GPIOInstance *instance)
{
    instance->GPIOx->output &= ~instance->GPIO_Pin;
    instance->pin_state = GPIO_PIN_RESET;
}

void GPIOToggel(GPIOInstance *instance)
{
    instance->GPIOx->output ^= instance->GPIO_Pin;
    instance->pin_state =
        ((instance->GPIOx->output & instance->GPIO_Pin) != 0U)
            ? GPIO_PIN_SET
            : GPIO_PIN_RESET;
}

int main(void)
{
    GPIO_TypeDef gpio = {0};
    Electromagnet_Device_t active_high = {0};
    Electromagnet_Device_t active_low = {0};
    Electromagnet_Device_t unused = {0};
    const uint32_t high_pin = 1UL << 1U;
    const uint32_t low_pin = 1UL << 2U;

    gpio.output = high_pin;
    assert(Electromagnet_Init(
        &active_high, &gpio, high_pin, GPIO_PIN_SET));
    assert((gpio.output & high_pin) == 0U);
    Electromagnet_On(&active_high);
    assert((gpio.output & high_pin) != 0U);
    Electromagnet_Toggle(&active_high);
    assert((gpio.output & high_pin) == 0U);
    Electromagnet_On(&active_high);
    Electromagnet_Off(&active_high);
    assert((gpio.output & high_pin) == 0U);

    assert(Electromagnet_Init(
        &active_low, &gpio, low_pin, GPIO_PIN_RESET));
    assert((gpio.output & low_pin) != 0U);
    Electromagnet_On(&active_low);
    assert((gpio.output & low_pin) == 0U);
    Electromagnet_Off(&active_low);
    assert((gpio.output & low_pin) != 0U);

    Electromagnet_On(NULL);
    Electromagnet_Off(&unused);
    Electromagnet_Toggle(&unused);
    assert(!Electromagnet_Init(NULL, &gpio, high_pin, GPIO_PIN_SET));
    assert(!Electromagnet_Init(&unused, NULL, high_pin, GPIO_PIN_SET));
    assert(!Electromagnet_Init(&unused, &gpio, 0U, GPIO_PIN_SET));
    assert(!Electromagnet_Init(
        &unused, &gpio, high_pin, (GPIO_PinState) 2U));

    fail_registration = true;
    assert(!Electromagnet_Init(
        &unused, &gpio, 1UL << 3U, GPIO_PIN_SET));
    return 0;
}
