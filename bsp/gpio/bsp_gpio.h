#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include "bsp_device.h"

#define GPIO_MX_DEVICE_NUM 10U

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

GPIOInstance *GPIORegister(GPIO_Init_Config_s *GPIO_config);
void GPIOUnregister(GPIOInstance *instance);
void GPIOToggel(GPIOInstance *_instance);
void GPIOSet(GPIOInstance *_instance);
void GPIOReset(GPIOInstance *_instance);
GPIO_PinState GPIORead(GPIOInstance *_instance);

/* Called by the maintained GPIO IRQ entry after reading the pending pin. */
void GPIOInterruptCallback(uint32_t gpio_pin);

#endif
