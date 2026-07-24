#include <assert.h>
#include <stddef.h>

#include "photoelectric.h"

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

GPIO_PinState GPIORead(GPIOInstance *instance)
{
    return ((instance->GPIOx->input & instance->GPIO_Pin) != 0U)
        ? GPIO_PIN_SET
        : GPIO_PIN_RESET;
}

int main(void)
{
    GPIO_TypeDef gpio = {0};
    Photoelectric_Device_t active_high = {0};
    Photoelectric_Device_t active_low = {0};
    Photoelectric_Device_t unused = {0};
    const uint32_t high_pin = 1UL << 4U;
    const uint32_t low_pin = 1UL << 5U;

    assert(Photoelectric_Init(
        &active_high, &gpio, high_pin, GPIO_PIN_SET));
    assert(Photoelectric_Init(
        &active_low, &gpio, low_pin, GPIO_PIN_RESET));

    assert(!Photoelectric_IsTriggered(&active_high));
    gpio.input |= high_pin;
    assert(Photoelectric_IsTriggered(&active_high));

    assert(Photoelectric_IsTriggered(&active_low));
    gpio.input |= low_pin;
    assert(!Photoelectric_IsTriggered(&active_low));

    assert(!Photoelectric_IsTriggered(NULL));
    assert(!Photoelectric_IsTriggered(&unused));
    assert(!Photoelectric_Init(NULL, &gpio, high_pin, GPIO_PIN_SET));
    assert(!Photoelectric_Init(&unused, NULL, high_pin, GPIO_PIN_SET));
    assert(!Photoelectric_Init(&unused, &gpio, 0U, GPIO_PIN_SET));
    assert(!Photoelectric_Init(
        &unused, &gpio, high_pin, (GPIO_PinState) 2U));

    fail_registration = true;
    assert(!Photoelectric_Init(
        &unused, &gpio, 1UL << 6U, GPIO_PIN_SET));
    return 0;
}
