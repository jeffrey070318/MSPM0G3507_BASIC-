#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "gray_sensor.h"
#include "line_follow.h"
#include "robot_def.h"

GPIO_TypeDef test_gray_port;

static GraySensorInstance sample;
static GraySensor_Init_Config_s captured_config;
static bool registration_fails;
static Device_Status_e update_status = DEVICE_OK;

GraySensorInstance *GraySensorRegister(
    const GraySensor_Init_Config_s *config)
{
    if (registration_fails || (config == NULL)) {
        return NULL;
    }
    captured_config = *config;
    return &sample;
}

Device_Status_e GraySensorUpdate(GraySensorInstance *sensor)
{
    assert(sensor == &sample);
    return update_status;
}

int main(void)
{
    LineFollow_Output_t output = {0};
    assert(LineFollowInit());
    assert(captured_config.active_state == GPIO_PIN_SET);

    LineFollowTask(false, 0.005f, &output);
    assert(!output.line_valid);
    assert(output.vx_mps == 0.0f);
    assert(output.wz_radps == 0.0f);

    sample.active_count = 2U;
    sample.offset = 0.3f;
    LineFollowTask(true, 0.005f, &output);
    assert(output.line_valid);
    assert(output.vx_mps == LINE_FOLLOW_BASE_SPEED_MPS);
    assert(output.wz_radps < 0.0f);

    sample.active_count = 0U;
    LineFollowTask(true, 0.005f, &output);
    assert(!output.line_valid);
    assert(output.vx_mps == 0.0f);
    assert(output.wz_radps == 0.0f);

    sample.active_count = LINE_FOLLOW_A_MARKER_ACTIVE_MIN;
    for (uint32_t i = 1U; i < LINE_FOLLOW_A_MARKER_DEBOUNCE_SAMPLES; i++) {
        LineFollowTask(true, 0.005f, &output);
        assert(!output.a_marker_event);
    }
    LineFollowTask(true, 0.005f, &output);
    assert(output.a_marker_event);
    LineFollowTask(true, 0.005f, &output);
    assert(!output.a_marker_event);

    sample.active_count = 1U;
    for (uint32_t i = 0U; i < LINE_FOLLOW_A_MARKER_REARM_SAMPLES; i++) {
        LineFollowTask(true, 0.005f, &output);
        assert(!output.a_marker_event);
    }

    sample.active_count = LINE_FOLLOW_A_MARKER_ACTIVE_MIN;
    for (uint32_t i = 1U; i < LINE_FOLLOW_A_MARKER_DEBOUNCE_SAMPLES; i++) {
        LineFollowTask(true, 0.005f, &output);
        assert(!output.a_marker_event);
    }
    LineFollowTask(true, 0.005f, &output);
    assert(output.a_marker_event);

    update_status = DEVICE_ERROR;
    LineFollowTask(true, 0.005f, &output);
    assert(!output.line_valid);

    return 0;
}
