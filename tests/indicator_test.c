#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "bsp_gpio.h"
#include "buzzer.h"
#include "led.h"
#include "ti_msp_dl_config.h"

GPIO_TypeDef indicator_gpio_b;

static GPIOInstance gpio_instances[4];
static uint32_t gpio_instance_count;

GPIOInstance *GPIORegister(GPIO_Init_Config_s *config)
{
    if ((config == NULL) || (gpio_instance_count >= 4U)) {
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
    LED_Device_t led3 = {0};
    LED_Device_t led4 = {0};
    LED_Device_t board_led = {0};
    LED_Device_t unused_led = {0};

    assert(LED_Init(&led3, LED_GPIO_PORT, LED_GPIO_LED3_PIN));
    assert(LED_Init(&led4, LED_GPIO_PORT, LED_GPIO_LED4_PIN));
    assert(LED_Init(
        &board_led, LED_GPIO_PORT, LED_GPIO_BOARD_LED_PIN));
    assert((indicator_gpio_b.output & (LED_GPIO_LED3_PIN |
        LED_GPIO_LED4_PIN | LED_GPIO_BOARD_LED_PIN)) == 0U);

    LED_On(&led3);
    assert((indicator_gpio_b.output & LED_GPIO_LED3_PIN) != 0U);
    LED_Toggle(&led3);
    assert((indicator_gpio_b.output & LED_GPIO_LED3_PIN) == 0U);
    LED_On(&board_led);
    LED_Off(&board_led);
    assert((indicator_gpio_b.output & LED_GPIO_BOARD_LED_PIN) == 0U);

    LED_On(&unused_led);
    LED_Off(NULL);
    LED_Toggle(NULL);
    assert(!LED_Init(NULL, LED_GPIO_PORT, LED_GPIO_LED3_PIN));
    assert(!LED_Init(&unused_led, NULL, LED_GPIO_LED3_PIN));
    assert(!LED_Init(&unused_led, LED_GPIO_PORT, 0U));

    assert(Buzzer_Init());
    assert((indicator_gpio_b.output & BUZZER_GPIO_BUZZER_PIN) == 0U);
    Buzzer_On();
    assert((indicator_gpio_b.output & BUZZER_GPIO_BUZZER_PIN) != 0U);
    Buzzer_Toggle();
    assert((indicator_gpio_b.output & BUZZER_GPIO_BUZZER_PIN) == 0U);
    Buzzer_On();
    Buzzer_Off();
    assert((indicator_gpio_b.output & BUZZER_GPIO_BUZZER_PIN) == 0U);
    return 0;
}
