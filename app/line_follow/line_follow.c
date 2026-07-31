#include "line_follow.h"

#include <stddef.h>
#include <stdint.h>

#include "gray_sensor.h"
#include "pid.h"
#include "robot_def.h"
#include "ti_msp_dl_config.h"

static GraySensorInstance *g_sensor;
static PID_Controller_t g_line_pid;
static uint32_t g_marker_active_samples;
static uint32_t g_marker_clear_samples;
static bool g_marker_latched;
static uint32_t g_lost_line_samples;
static float g_last_wz_radps;
static bool g_has_last_line_command;

volatile bool line_follow_debug_enabled;
volatile uint8_t line_follow_debug_raw_value;
volatile uint8_t line_follow_debug_active_count;
volatile float line_follow_debug_offset;
volatile float line_follow_debug_wz_radps;

static void LineFollowClearOutput(LineFollow_Output_t *output)
{
    if (output != NULL) {
        *output = (LineFollow_Output_t) {0};
    }
}

static void LineFollowResetControl(void)
{
    PID_ControllerReset(&g_line_pid);
    g_lost_line_samples = 0U;
    g_last_wz_radps = 0.0f;
    g_has_last_line_command = false;
}

static bool LineFollowUpdateMarker(uint8_t active_count)
{
    bool event = false;
    if (active_count >= LINE_FOLLOW_A_MARKER_ACTIVE_MIN) {
        g_marker_clear_samples = 0U;
        if (g_marker_active_samples < LINE_FOLLOW_A_MARKER_DEBOUNCE_SAMPLES) {
            g_marker_active_samples++;
        }
        if (!g_marker_latched &&
            (g_marker_active_samples >=
                LINE_FOLLOW_A_MARKER_DEBOUNCE_SAMPLES)) {
            g_marker_latched = true;
            event = true;
        }
    } else {
        g_marker_active_samples = 0U;
        if (g_marker_latched) {
            if (g_marker_clear_samples <
                LINE_FOLLOW_A_MARKER_REARM_SAMPLES) {
                g_marker_clear_samples++;
            }
            if (g_marker_clear_samples >=
                LINE_FOLLOW_A_MARKER_REARM_SAMPLES) {
                g_marker_latched = false;
            }
        }
    }
    return event;
}

bool LineFollowInit(void)
{
    GraySensor_Init_Config_s sensor_config = {
        .ad0_port = GRAY_SENSOR_GPIO_PORT,
        .ad0_pin = GRAY_SENSOR_GPIO_AD0_PIN,
        .ad1_port = GRAY_SENSOR_GPIO_PORT,
        .ad1_pin = GRAY_SENSOR_GPIO_AD1_PIN,
        .ad2_port = GRAY_SENSOR_GPIO_PORT,
        .ad2_pin = GRAY_SENSOR_GPIO_AD2_PIN,
        .out_port = GRAY_SENSOR_GPIO_PORT,
        .out_pin = GRAY_SENSOR_GPIO_OUT_PIN,
        .active_state = LINE_FOLLOW_SENSOR_ACTIVE_STATE,
        .channel_order = LINE_FOLLOW_SENSOR_CHANNEL_ORDER,
        .settle_time_us = LINE_FOLLOW_SENSOR_SETTLE_US,
        .id = NULL,
    };
    PID_Config_t pid_config = {
        .kp = LINE_FOLLOW_KP,
        .ki = LINE_FOLLOW_KI,
        .kd = LINE_FOLLOW_KD,
        .output_limit = LINE_FOLLOW_MAX_WZ_RADPS,
        .integral_limit = LINE_FOLLOW_MAX_IOUT,
        .deadband = LINE_FOLLOW_DEADBAND,
        .derivative_on_measurement = true,
    };

    g_sensor = GraySensorRegister(&sensor_config);
    g_marker_active_samples = 0U;
    g_marker_clear_samples = 0U;
    g_marker_latched = false;
    g_lost_line_samples = 0U;
    g_last_wz_radps = 0.0f;
    g_has_last_line_command = false;
    line_follow_debug_enabled = false;
    line_follow_debug_raw_value = 0U;
    line_follow_debug_active_count = 0U;
    line_follow_debug_offset = 0.0f;
    line_follow_debug_wz_radps = 0.0f;

    return (g_sensor != NULL) && PID_ControllerInit(&g_line_pid, &pid_config);
}

void LineFollowTask(bool enabled, float dt_seconds,
    LineFollow_Output_t *output)
{
    LineFollowClearOutput(output);
    line_follow_debug_enabled = enabled;
    line_follow_debug_raw_value = 0U;
    line_follow_debug_active_count = 0U;
    line_follow_debug_offset = 0.0f;
    line_follow_debug_wz_radps = 0.0f;

    if ((output == NULL) || (g_sensor == NULL) || !enabled ||
        !(dt_seconds > 0.0f)) {
        LineFollowResetControl();
        return;
    }

    if (GraySensorUpdate(g_sensor) != DEVICE_OK) {
        LineFollowResetControl();
        return;
    }

    line_follow_debug_raw_value = g_sensor->raw_value;
    line_follow_debug_active_count = g_sensor->active_count;
    line_follow_debug_offset = g_sensor->offset;
    output->a_marker_event =
        LineFollowUpdateMarker(g_sensor->active_count);
    if (g_sensor->active_count == 0U) {
        if (g_has_last_line_command &&
            (g_lost_line_samples < LINE_FOLLOW_LOST_LINE_HOLD_SAMPLES)) {
            g_lost_line_samples++;
            output->line_valid = true;
            output->vx_mps = LINE_FOLLOW_BASE_SPEED_MPS;
            output->wz_radps = g_last_wz_radps;
            line_follow_debug_wz_radps = output->wz_radps;
            return;
        }
        LineFollowResetControl();
        return;
    }

    g_lost_line_samples = 0U;
    output->line_valid = true;
    output->vx_mps = LINE_FOLLOW_BASE_SPEED_MPS;
    output->wz_radps = PID_ControllerUpdate(
        &g_line_pid, 0.0f, g_sensor->offset, dt_seconds);
    g_last_wz_radps = output->wz_radps;
    g_has_last_line_command = true;
    line_follow_debug_wz_radps = output->wz_radps;
}
