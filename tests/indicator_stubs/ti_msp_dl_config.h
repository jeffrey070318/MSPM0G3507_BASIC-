#ifndef TEST_INDICATOR_TI_MSP_DL_CONFIG_H
#define TEST_INDICATOR_TI_MSP_DL_CONFIG_H

#include "bsp_device.h"

extern GPIO_TypeDef indicator_gpio_b;

#define LED_GPIO_PORT (&indicator_gpio_b)
#define LED_GPIO_LED3_PIN (1UL << 25U)
#define LED_GPIO_LED4_PIN (1UL << 26U)
#define LED_GPIO_BOARD_LED_PIN (1UL << 22U)

#define BUZZER_GPIO_PORT (&indicator_gpio_b)
#define BUZZER_GPIO_BUZZER_PIN (1UL << 13U)

#endif
