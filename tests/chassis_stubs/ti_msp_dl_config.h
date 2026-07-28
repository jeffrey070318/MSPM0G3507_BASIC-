#ifndef TEST_CHASSIS_TI_MSP_DL_CONFIG_H
#define TEST_CHASSIS_TI_MSP_DL_CONFIG_H

#include "motor.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern Encoder_Device_t hencoder_left;
extern Encoder_Device_t hencoder_right;

extern GPIO_TypeDef test_raw_gpio_a;
extern GPIO_TypeDef test_raw_gpio_b;
extern GPIO_TypeDef test_motor_gpio_a;
extern GPIO_TypeDef test_motor_gpio_b;

#define GPIOA (&test_raw_gpio_a)
#define GPIOB (&test_raw_gpio_b)
#define GPIO_PIN_17 (1UL << 17)
#define GPIO_PIN_4  (1UL << 4)

#define MOTOR_GPIO_AIN1_PORT (&test_motor_gpio_a)
#define MOTOR_GPIO_AIN1_PIN  (1UL << 7)
#define MOTOR_GPIO_BIN1_PORT (&test_motor_gpio_b)
#define MOTOR_GPIO_BIN1_PIN  (1UL << 9)

#endif
