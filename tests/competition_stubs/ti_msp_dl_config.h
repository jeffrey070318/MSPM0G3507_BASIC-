#ifndef TEST_COMPETITION_SYSCFG_H
#define TEST_COMPETITION_SYSCFG_H

#include "key.h"

extern GPIO_TypeDef test_key_port;

#define KEY_GPIO_KEY1_PORT (&test_key_port)
#define KEY_GPIO_KEY1_PIN  (1UL << 0U)
#define KEY_GPIO_KEY2_PORT (&test_key_port)
#define KEY_GPIO_KEY2_PIN  (1UL << 1U)

#endif
