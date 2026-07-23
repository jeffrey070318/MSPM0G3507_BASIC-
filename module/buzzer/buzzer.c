#include "buzzer.h"

#include <stddef.h>

#include "bsp_gpio.h"
#include "ti_msp_dl_config.h"

static GPIOInstance *buzzer_instance;

bool Buzzer_Init(void)
{
    if (buzzer_instance != NULL) {
        return true;
    }

    GPIO_Init_Config_s config = {
        .GPIOx = BUZZER_GPIO_PORT,
        .GPIO_Pin = BUZZER_GPIO_BUZZER_PIN,
        .pin_state = GPIO_PIN_RESET,
        .exti_mode = GPIO_EXTI_MODE_NONE,
        .gpio_model_callback = NULL,
        .id = NULL,
    };
    buzzer_instance = GPIORegister(&config);
    if (buzzer_instance == NULL) {
        return false;
    }

    Buzzer_Off();
    return true;
}

void Buzzer_On(void)
{
    if (buzzer_instance != NULL) {
        GPIOSet(buzzer_instance);
    }
}

void Buzzer_Off(void)
{
    if (buzzer_instance != NULL) {
        GPIOReset(buzzer_instance);
    }
}

void Buzzer_Toggle(void)
{
    if (buzzer_instance != NULL) {
        GPIOToggel(buzzer_instance);
    }
}
