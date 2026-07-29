#ifndef TEST_LINE_FOLLOW_SYSCFG_H
#define TEST_LINE_FOLLOW_SYSCFG_H

#include "gray_sensor.h"

extern GPIO_TypeDef test_gray_port;

#define GRAY_SENSOR_GPIO_PORT     (&test_gray_port)
#define GRAY_SENSOR_GPIO_AD0_PIN  (1UL << 0U)
#define GRAY_SENSOR_GPIO_AD1_PIN  (1UL << 1U)
#define GRAY_SENSOR_GPIO_AD2_PIN  (1UL << 2U)
#define GRAY_SENSOR_GPIO_OUT_PIN  (1UL << 3U)

#endif
