#ifndef TEST_BALL_BALANCE_KEY_H
#define TEST_BALL_BALANCE_KEY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct GPIO_TypeDef {
    uint32_t marker;
} GPIO_TypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET,
} GPIO_PinState;

typedef struct {
    uint32_t marker;
} KEY_Device_t;

bool KEY_Init(KEY_Device_t *key, GPIO_TypeDef *gpio_port,
    uint32_t gpio_pin, GPIO_PinState active_state);
bool KEY_IsPressed(KEY_Device_t *key);

#endif
