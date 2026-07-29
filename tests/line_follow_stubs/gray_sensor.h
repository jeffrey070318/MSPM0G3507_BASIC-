#ifndef TEST_GRAY_SENSOR_H
#define TEST_GRAY_SENSOR_H

#include <stdint.h>

typedef struct GPIO_TypeDef {
    uint32_t marker;
} GPIO_TypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET,
} GPIO_PinState;

typedef enum {
    DEVICE_OK = 0,
    DEVICE_ERROR,
} Device_Status_e;

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
    uint8_t raw_value;
    uint8_t active_count;
    float offset;
} GraySensorInstance;

GraySensorInstance *GraySensorRegister(
    const GraySensor_Init_Config_s *config);
Device_Status_e GraySensorUpdate(GraySensorInstance *sensor);

#endif
