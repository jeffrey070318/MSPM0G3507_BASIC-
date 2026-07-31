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

extern volatile bool line_follow_debug_enabled;
extern volatile uint8_t line_follow_debug_raw_value;
extern volatile uint8_t line_follow_debug_active_count;
extern volatile float line_follow_debug_offset;
extern volatile float line_follow_debug_wz_radps;

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
    assert(!line_follow_debug_enabled);
    assert(line_follow_debug_raw_value == 0U);
    assert(line_follow_debug_active_count == 0U);
    assert(line_follow_debug_offset == 0.0f);
    assert(line_follow_debug_wz_radps == 0.0f);
    assert(!output.line_valid);
    assert(output.vx_mps == 0.0f);
    assert(output.wz_radps == 0.0f);

    sample.active_count = 2U;
    sample.raw_value = 0x18U;
    sample.offset = 0.3f;
    LineFollowTask(true, 0.005f, &output);
    assert(line_follow_debug_enabled);
    assert(line_follow_debug_raw_value == sample.raw_value);
    assert(line_follow_debug_active_count == sample.active_count);
    assert(line_follow_debug_offset == sample.offset);
    assert(line_follow_debug_wz_radps == output.wz_radps);
    assert(output.line_valid);
    assert(output.vx_mps == LINE_FOLLOW_BASE_SPEED_MPS);
    assert(output.wz_radps < 0.0f);
    const float last_wz_radps = output.wz_radps;

    sample.active_count = 0U;
    sample.raw_value = 0U;
    sample.offset = 0.0f;
    LineFollowTask(true, 0.005f, &output);
    assert(line_follow_debug_active_count == 0U);
    assert(line_follow_debug_wz_radps == last_wz_radps);
    assert(output.line_valid);
    assert(output.vx_mps == LINE_FOLLOW_BASE_SPEED_MPS);
    assert(output.wz_radps == last_wz_radps);
    for (uint32_t i = 1U; i < LINE_FOLLOW_LOST_LINE_HOLD_SAMPLES; i++) {
        LineFollowTask(true, 0.005f, &output);
        assert(output.line_valid);
    }
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
    assert(line_follow_debug_raw_value == 0U);
    assert(line_follow_debug_active_count == 0U);
    assert(line_follow_debug_offset == 0.0f);
    assert(line_follow_debug_wz_radps == 0.0f);
    assert(!output.line_valid);

    return 0;
}
