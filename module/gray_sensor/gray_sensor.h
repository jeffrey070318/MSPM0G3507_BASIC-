#ifndef GRAY_SENSOR_H
#define GRAY_SENSOR_H

#include <stdint.h>

#include "bsp_gpio.h"

#define GRAY_SENSOR_COUNT 8U

typedef struct {
    GPIOInstance *sensor;
    uint8_t value;
} Gray_Sensor_Pin_t;

typedef struct {
    Gray_Sensor_Pin_t sensors[GRAY_SENSOR_COUNT];
    GPIO_PinState active_state;
    uint8_t active_count;
} Gray_Sensor_t;

void Gray_Sensor_Init(Gray_Sensor_t *sensor);
float Gray_Sensor_Get_Offset(Gray_Sensor_t *sensor);
uint8_t Gray_Sensor_Get_Node(Gray_Sensor_t *sensor);

#endif
