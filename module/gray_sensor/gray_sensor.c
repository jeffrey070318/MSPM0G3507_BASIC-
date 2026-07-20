#include "gray_sensor.h"

#include "bsp_dwt.h"
#include "bsp_memory.h"

#include <string.h>

#define GRAY_SENSOR_DEFAULT_SETTLE_TIME_US 1U

static GraySensorInstance *gray_sensor_instance[GRAY_SENSOR_DEVICE_CNT];
static uint8_t gray_sensor_idx;

static uint8_t GraySensorConfigIsValid(
    const GraySensor_Init_Config_s *config)
{
    return (config != NULL) &&
           (config->ad0_port != NULL) && (config->ad0_pin != 0U) &&
           (config->ad1_port != NULL) && (config->ad1_pin != 0U) &&
           (config->ad2_port != NULL) && (config->ad2_pin != 0U) &&
           (config->out_port != NULL) && (config->out_pin != 0U) &&
           (config->active_state <= GPIO_PIN_SET) &&
           (config->channel_order <= GRAY_SENSOR_CHANNEL_1_ON_RIGHT) &&
           (gray_sensor_idx < GRAY_SENSOR_DEVICE_CNT);
}

static GPIOInstance *GraySensorRegisterPin(
    GPIO_TypeDef *port, uint32_t pin, GPIO_PinState initial_state)
{
    GPIO_Init_Config_s gpio_config = {
        .GPIOx = port,
        .GPIO_Pin = pin,
        .pin_state = initial_state,
        .exti_mode = GPIO_EXTI_MODE_NONE,
        .gpio_model_callback = NULL,
        .id = NULL,
    };
    return GPIORegister(&gpio_config);
}

static void GraySensorWriteAddress(
    GraySensorInstance *sensor, uint8_t address)
{
    for (uint8_t bit = 0U; bit < 3U; bit++) {
        if ((address & (1U << bit)) != 0U) {
            GPIOSet(sensor->address_pin[bit]);
        } else {
            GPIOReset(sensor->address_pin[bit]);
        }
    }
}

static float GraySensorGetChannelWeight(
    const GraySensorInstance *sensor, uint8_t channel_index)
{
    static const float left_to_right_weight[GRAY_SENSOR_CHANNEL_COUNT] = {
        -0.70f, -0.50f, -0.30f, -0.10f,
         0.10f,  0.30f,  0.50f,  0.70f,
    };

    if (sensor->channel_order == GRAY_SENSOR_CHANNEL_1_ON_RIGHT) {
        channel_index =
            (uint8_t) (GRAY_SENSOR_CHANNEL_COUNT - 1U - channel_index);
    }
    return left_to_right_weight[channel_index];
}

GraySensorInstance *GraySensorRegister(
    const GraySensor_Init_Config_s *config)
{
    if (!GraySensorConfigIsValid(config)) {
        return NULL;
    }

    GraySensorInstance *sensor =
        (GraySensorInstance *) BSPMalloc(sizeof(GraySensorInstance));
    if (sensor == NULL) {
        return NULL;
    }
    memset(sensor, 0, sizeof(GraySensorInstance));

    sensor->address_pin[0] = GraySensorRegisterPin(
        config->ad0_port, config->ad0_pin, GPIO_PIN_RESET);
    sensor->address_pin[1] = GraySensorRegisterPin(
        config->ad1_port, config->ad1_pin, GPIO_PIN_RESET);
    sensor->address_pin[2] = GraySensorRegisterPin(
        config->ad2_port, config->ad2_pin, GPIO_PIN_RESET);
    sensor->output_pin = GraySensorRegisterPin(
        config->out_port, config->out_pin, GPIO_PIN_RESET);

    if ((sensor->address_pin[0] == NULL) ||
        (sensor->address_pin[1] == NULL) ||
        (sensor->address_pin[2] == NULL) ||
        (sensor->output_pin == NULL)) {
        GPIOUnregister(sensor->address_pin[0]);
        GPIOUnregister(sensor->address_pin[1]);
        GPIOUnregister(sensor->address_pin[2]);
        GPIOUnregister(sensor->output_pin);
        BSPFree(sensor);
        return NULL;
    }

    sensor->active_state = config->active_state;
    sensor->channel_order = config->channel_order;
    sensor->settle_time_us =
        (config->settle_time_us != 0U)
            ? config->settle_time_us
            : GRAY_SENSOR_DEFAULT_SETTLE_TIME_US;
    sensor->id = config->id;

    gray_sensor_instance[gray_sensor_idx++] = sensor;
    Gray_Sensor_Init(sensor);
    return sensor;
}

Device_Status_e GraySensorUpdate(GraySensorInstance *sensor)
{
    if ((sensor == NULL) || (sensor->output_pin == NULL)) {
        return DEVICE_ERROR;
    }

    sensor->raw_value = 0U;
    sensor->active_count = 0U;
    sensor->offset = 0.0f;

    for (uint8_t channel = 0U;
         channel < GRAY_SENSOR_CHANNEL_COUNT; channel++) {
        GraySensorWriteAddress(sensor, channel);
        DWT_Delay((float) sensor->settle_time_us / 1000000.0f);

        uint8_t active =
            (GPIORead(sensor->output_pin) == sensor->active_state)
                ? 1U
                : 0U;
        sensor->channel_value[channel] = active;
        if (active != 0U) {
            sensor->raw_value |= (uint8_t) (1U << channel);
            sensor->active_count++;
            sensor->offset += GraySensorGetChannelWeight(sensor, channel);
        }
    }

    if (sensor->active_count != 0U) {
        sensor->offset /= (float) sensor->active_count;
    }
    return DEVICE_OK;
}

uint8_t GraySensorGetRawValue(const GraySensorInstance *sensor)
{
    return (sensor != NULL) ? sensor->raw_value : 0U;
}

uint8_t GraySensorGetChannelValue(
    const GraySensorInstance *sensor, uint8_t channel)
{
    if ((sensor == NULL) || (channel == 0U) ||
        (channel > GRAY_SENSOR_CHANNEL_COUNT)) {
        return 0U;
    }
    return sensor->channel_value[channel - 1U];
}

float GraySensorGetOffset(GraySensorInstance *sensor)
{
    if (GraySensorUpdate(sensor) != DEVICE_OK) {
        return 0.0f;
    }
    return sensor->offset;
}

void Gray_Sensor_Init(Gray_Sensor_t *sensor)
{
    if (sensor == NULL) {
        return;
    }

    sensor->raw_value = 0U;
    sensor->active_count = 0U;
    sensor->offset = 0.0f;
    memset(sensor->channel_value, 0, sizeof(sensor->channel_value));
    GraySensorWriteAddress(sensor, 0U);
}

float Gray_Sensor_Get_Offset(Gray_Sensor_t *sensor)
{
    return GraySensorGetOffset(sensor);
}
