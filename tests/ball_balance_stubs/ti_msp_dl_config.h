#ifndef TEST_BALL_BALANCE_SYSCFG_H
#define TEST_BALL_BALANCE_SYSCFG_H

#include "key.h"

extern GPIO_TypeDef test_key_port;

#define KEY_GPIO_KEY3_PORT (&test_key_port)
#define KEY_GPIO_KEY3_PIN  (1UL << 2U)

#endif
