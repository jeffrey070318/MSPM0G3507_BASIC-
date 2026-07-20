#include "gray_sensor.h"

static const float gray_sensor_weights[GRAY_SENSOR_COUNT] = {
    -0.70f, -0.50f, -0.30f, -0.10f,
     0.10f,  0.30f,  0.50f,  0.70f,
};

void Gray_Sensor_Init(Gray_Sensor_t *sensor)
{
    if (sensor == NULL) {
        return;
    }

    sensor->active_count = 0U;
    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; i++) {
        sensor->sensors[i].value = 0U;
    }
}

static void GraySensorRead(Gray_Sensor_t *sensor)
{
    sensor->active_count = 0U;
    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; i++) {
        if (sensor->sensors[i].sensor == NULL) {
            sensor->sensors[i].value = 0U;
            continue;
        }

        sensor->sensors[i].value =
            (GPIORead(sensor->sensors[i].sensor) == sensor->active_state)
                ? 1U
                : 0U;
        sensor->active_count += sensor->sensors[i].value;
    }
}

float Gray_Sensor_Get_Offset(Gray_Sensor_t *sensor)
{
    if (sensor == NULL) {
        return 0.0f;
    }

    GraySensorRead(sensor);
    float weighted_sum = 0.0f;
    for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; i++) {
        if (sensor->sensors[i].value != 0U) {
            weighted_sum += gray_sensor_weights[i];
        }
    }

    if (sensor->active_count == 0U) {
        return 0.0f;
    }
    return weighted_sum / (float) sensor->active_count;
}

uint8_t Gray_Sensor_Get_Node(Gray_Sensor_t *sensor)
{
    (void) sensor;

    /* Track-node semantics depend on the final course definition. */
    return 0U;
}
