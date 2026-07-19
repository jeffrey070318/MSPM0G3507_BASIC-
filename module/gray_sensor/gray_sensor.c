#include "gray_sensor.h"
#include "bsp_encoder.h"
#include <stdbool.h>

extern Encoder_Device_t ENCODER_L;
extern Encoder_Device_t ENCODER_R;

void Gray_Sensor_Init(Gray_Sensor_t *sensor)
{
    if (sensor == NULL) return;
    for (int i = 0; i < GRAY_SENSOR_COUNT; i++)
        sensor->sensors[i].value = 0;
}

static uint8_t Read_Sensor(GPIOInstance *dev)
{
    return (GPIORead(dev) == GPIO_PIN_SET) ? 1 : 0;
}

static void Read_Sensors(Gray_Sensor_t *sensor)
{
    if (sensor == NULL) return;
    for (int i = 0; i < GRAY_SENSOR_COUNT; i++) {
        if (sensor->sensors[i].sensor == NULL) {
            sensor->sensors[i].value = 0;
            continue;
        }
        sensor->sensors[i].value = Read_Sensor(sensor->sensors[i].sensor);
    }
}

float Gray_Sensor_Get_Offset(Gray_Sensor_t *sensor)
{
    if (sensor == NULL) return 0.0f;

    Read_Sensors(sensor);
    float offset = 0.0f;
    int weighted_sum = 0;
    const float weights[8] = {-0.70f, -0.50f, -0.30f, -0.10f, 0.10f, 0.30f, 0.50f, 0.70f};

    for (int i = 0; i < GRAY_SENSOR_COUNT; i++) {
        if (sensor->sensors[i].value == 1) {
            offset += weights[i];
            weighted_sum++;
        }
    }
    if (weighted_sum > 0) offset /= (float)weighted_sum;
    return offset;
}

uint8_t Gray_Sensor_Get_Node(Gray_Sensor_t *sensor)
{
    static bool had_line_last_cycle = false;
    static bool a_distance_tracking = false;
    static int32_t a_start_cnt_l = 0, a_start_cnt_r = 0;
    const float b_distance_m = 0.8f;
    const uint8_t d_confirm_cycles = 1;
    static uint8_t d_lost_count = 0;

    if (sensor == NULL) return 0;

    float offset_now = Gray_Sensor_Get_Offset(sensor);
    (void)offset_now;

    bool has_line_now = false;
    for (int i = 0; i < GRAY_SENSOR_COUNT; i++) {
        if (sensor->sensors[i].value == 1) { has_line_now = true; break; }
    }

    bool detected_A = (!had_line_last_cycle && has_line_now);
    if (detected_A) {
        a_start_cnt_l = Encoder_Get_Total(&ENCODER_L);
        a_start_cnt_r = Encoder_Get_Total(&ENCODER_R);
        a_distance_tracking = true;
    }

    bool detected_B = false;
    if (a_distance_tracking) {
        int32_t dl = Encoder_Get_Total(&ENCODER_L) - a_start_cnt_l;
        int32_t dr = Encoder_Get_Total(&ENCODER_R) - a_start_cnt_r;
        if (dl < 0) dl = -dl;
        if (dr < 0) dr = -dr;
        float traveled = ((float)dl + (float)dr) * 0.5f * PULSE_TO_METER;
        detected_B = (traveled >= b_distance_m);
    }

    if (had_line_last_cycle && !has_line_now) {
        if (d_lost_count < 255) d_lost_count++;
    } else {
        d_lost_count = 0;
    }
    bool detected_D = (d_lost_count >= d_confirm_cycles);
    had_line_last_cycle = has_line_now;

    if (detected_A) {
        d_lost_count = 0;
        return 3;
    } else if (detected_B) {
        a_distance_tracking = false;
        d_lost_count = 0;
        return 1;
    } else if (detected_D) {
        d_lost_count = 0;
        return 2;
    }
    return 0;
}
