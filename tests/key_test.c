#include <assert.h>
#include <stddef.h>

#include "key.h"

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
    KEY_Device_t active_low_key = {0};
    KEY_Device_t active_high_key = {0};
    KEY_Device_t uninitialized_key = {0};

    assert(KEY_Init(&active_low_key, &gpio, 1UL << 1U, GPIO_PIN_RESET));
    assert(KEY_Init(&active_high_key, &gpio, 1UL << 2U, GPIO_PIN_SET));

    gpio.input = (1UL << 1U);
    assert(!KEY_IsPressed(&active_low_key));
    gpio.input &= ~(1UL << 1U);
    assert(KEY_IsPressed(&active_low_key));

    assert(!KEY_IsPressed(&active_high_key));
    gpio.input |= 1UL << 2U;
    assert(KEY_IsPressed(&active_high_key));

    assert(!KEY_IsPressed(NULL));
    assert(!KEY_IsPressed(&uninitialized_key));
    assert(!KEY_Init(NULL, &gpio, 1UL, GPIO_PIN_RESET));
    assert(!KEY_Init(&uninitialized_key, NULL, 1UL, GPIO_PIN_RESET));
    assert(!KEY_Init(&uninitialized_key, &gpio, 0U, GPIO_PIN_RESET));
    assert(!KEY_Init(
        &uninitialized_key, &gpio, 1UL, (GPIO_PinState) 2U));

    fail_registration = true;
    assert(!KEY_Init(
        &uninitialized_key, &gpio, 1UL << 3U, GPIO_PIN_RESET));
    return 0;
}
