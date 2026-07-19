#ifndef __GRAY_SENSOR_H
#define __GRAY_SENSOR_H

#include <stdint.h>
#include "bsp_gpio.h"

#define GRAY_SENSOR_COUNT 8

#ifndef PULSE_TO_METER
#define PULSE_TO_METER 0.001f
#endif

typedef struct
{
    GPIOInstance *sensor;
    uint8_t       value;
} Gray_Sensor_Pin_t;

typedef struct
{
    Gray_Sensor_Pin_t sensors[GRAY_SENSOR_COUNT];
} Gray_Sensor_t;

void    Gray_Sensor_Init(Gray_Sensor_t *sensor);
float   Gray_Sensor_Get_Offset(Gray_Sensor_t *sensor);
uint8_t Gray_Sensor_Get_Node(Gray_Sensor_t *sensor);

#endif
