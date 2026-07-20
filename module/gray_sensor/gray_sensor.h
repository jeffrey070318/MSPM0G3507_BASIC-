#ifndef GRAY_SENSOR_H
#define GRAY_SENSOR_H

#include <stdint.h>

#include "bsp_gpio.h"

#define GRAY_SENSOR_CHANNEL_COUNT 8U
#define GRAY_SENSOR_DEVICE_CNT    2U

typedef enum {
    GRAY_SENSOR_CHANNEL_1_ON_LEFT = 0,
    GRAY_SENSOR_CHANNEL_1_ON_RIGHT,
} GraySensor_Channel_Order_e;

typedef struct {
    GPIO_TypeDef *ad0_port;
    uint32_t ad0_pin;
    GPIO_TypeDef *ad1_port;
    uint32_t ad1_pin;
    GPIO_TypeDef *ad2_port;
    uint32_t ad2_pin;
    GPIO_TypeDef *out_port;
    uint32_t out_pin;
    GPIO_PinState active_state;
    GraySensor_Channel_Order_e channel_order;
    uint16_t settle_time_us;
    void *id;
} GraySensor_Init_Config_s;

typedef struct {
    GPIOInstance *address_pin[3];
    GPIOInstance *output_pin;
    GPIO_PinState active_state;
    GraySensor_Channel_Order_e channel_order;
    uint16_t settle_time_us;
    uint8_t channel_value[GRAY_SENSOR_CHANNEL_COUNT];
    uint8_t raw_value;
    uint8_t active_count;
    float offset;
    void *id;
} GraySensorInstance;

typedef GraySensorInstance Gray_Sensor_t;

GraySensorInstance *GraySensorRegister(
    const GraySensor_Init_Config_s *config);
Device_Status_e GraySensorUpdate(GraySensorInstance *sensor);
uint8_t GraySensorGetRawValue(const GraySensorInstance *sensor);
uint8_t GraySensorGetChannelValue(
    const GraySensorInstance *sensor, uint8_t channel);
float GraySensorGetOffset(GraySensorInstance *sensor);

/* Compatibility names retained for the first smart-car implementation. */
void Gray_Sensor_Init(Gray_Sensor_t *sensor);
float Gray_Sensor_Get_Offset(Gray_Sensor_t *sensor);

#endif
