#include <assert.h>
#include <stddef.h>

#include "bsp_gpio.h"
#include "buzzer.h"
#include "led.h"
#include "ti_msp_dl_config.h"

GPIO_TypeDef indicator_gpio_b;

GPIOInstance *GPIORegister(GPIO_Init_Config_s *config)
{
    (void) config;
    return NULL;
}

void GPIOSet(GPIOInstance *instance)
{
    (void) instance;
}

void GPIOReset(GPIOInstance *instance)
{
    (void) instance;
}

void GPIOToggel(GPIOInstance *instance)
{
    (void) instance;
}

int main(void)
{
    LED_Device_t led = {0};

    assert(!LED_Init(&led, LED_GPIO_PORT, LED_GPIO_LED3_PIN));
    assert(!Buzzer_Init());

    LED_On(&led);
    LED_Off(&led);
    LED_Toggle(&led);
    Buzzer_On();
    Buzzer_Off();
    Buzzer_Toggle();
    return 0;
}
