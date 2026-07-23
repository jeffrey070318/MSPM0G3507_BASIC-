#ifndef MOTOR_TEST_STUB_BSP_GPIO_H
#define MOTOR_TEST_STUB_BSP_GPIO_H

#include "bsp_device.h"

typedef enum {
    GPIO_EXTI_MODE_RISING,
    GPIO_EXTI_MODE_FALLING,
    GPIO_EXTI_MODE_RISING_FALLING,
    GPIO_EXTI_MODE_NONE,
} GPIO_EXTI_MODE_e;

typedef struct tmpgpio {
    GPIO_TypeDef *GPIOx;
    GPIO_PinState pin_state;
    GPIO_EXTI_MODE_e exti_mode;
    uint32_t GPIO_Pin;
    void (*gpio_model_callback)(struct tmpgpio *);
    void *id;
} GPIOInstance;

typedef struct {
    GPIO_TypeDef *GPIOx;
    GPIO_PinState pin_state;
    GPIO_EXTI_MODE_e exti_mode;
    uint32_t GPIO_Pin;
    void (*gpio_model_callback)(GPIOInstance *);
    void *id;
} GPIO_Init_Config_s;

GPIOInstance *GPIORegister(GPIO_Init_Config_s *config);
void GPIOToggel(GPIOInstance *instance);
void GPIOSet(GPIOInstance *instance);
void GPIOReset(GPIOInstance *instance);
GPIO_PinState GPIORead(GPIOInstance *instance);

#endif
