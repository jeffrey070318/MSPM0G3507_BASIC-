#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "bsp_init.h"
#include "ball_balance.h"
#include "chassis.h"
#include "competition.h"
#include "framework_runtime.h"
#include "imu.h"
#include "line_follow.h"
#include "oled.h"
#include "robot.h"
#include "task.h"
#include "vofa.h"

extern volatile bool robot_oled_initialized;
extern volatile uint32_t robot_oled_refresh_count;
extern volatile uint32_t robot_oled_error_count;
void RobotOLEDTask(void);

volatile FrameworkBootStage_e framework_boot_stage;
volatile uint32_t framework_robot_heartbeat;
volatile uint32_t framework_main_heartbeat;

static Device_Status_e oled_init_status = DEVICE_ERROR;
static unsigned created_task_count;
static unsigned oled_init_count;
static unsigned oled_clear_count;
static unsigned oled_refresh_count;
static char oled_rows[5][22];

void BSPInit(void)
{
}

void __disable_irq(void)
{
}

void __enable_irq(void)
{
}

Device_Status_e IMU_Init(void)
{
    return DEVICE_OK;
}

Device_Status_e IMU_ReadAll(IMU_Data_t *data)
{
    (void) data;
    return DEVICE_ERROR;
}

Device_Status_e VOFA_Init(void)
{
    return DEVICE_OK;
}

Device_Status_e VOFA_JustFloatOutputDMA(
    const float *channels, uint8_t channel_count)
{
    (void) channels;
    (void) channel_count;
    return DEVICE_OK;
}

bool ChassisInit(void)
{
    return true;
}

void ChassisTask(const Chassis_Command_t *command,
    float dt_seconds, Chassis_Status_t *status)
{
    (void) command;
    (void) dt_seconds;
    *status = (Chassis_Status_t) {
        .left_target_counts_s = 123.0f,
        .left_measured_counts_s = 120.0f,
        .right_target_counts_s = 130.0f,
        .right_measured_counts_s = 127.0f,
        .enabled = true,
    };
}

bool LineFollowInit(void)
{
    return true;
}

void LineFollowTask(bool enabled, float dt_seconds,
    LineFollow_Output_t *output)
{
    (void) enabled;
    (void) dt_seconds;
    output->line_valid = true;
}

bool BallBalanceInit(void)
{
    return true;
}

void BallBalanceTask(const BallBalance_Command_t *command,
    uint32_t now_ms, float dt_seconds, BallBalance_Status_t *status)
{
    (void) command;
    (void) now_ms;
    (void) dt_seconds;
    *status = (BallBalance_Status_t) {
        .measured_position = 42.0f,
        .step_position = 321,
        .vision_valid = true,
        .enabled = true,
    };
}

bool CompetitionInit(void)
{
    return true;
}

void CompetitionTask(uint32_t now_ms, bool app_ready,
    const LineFollow_Output_t *line_follow,
    const BallBalance_Status_t *ball_balance,
    Competition_Output_t *output)
{
    (void) now_ms;
    assert(app_ready);
    output->status = (Competition_Status_t) {
        .state = COMPETITION_RUNNING,
        .elapsed_ms = 1234U,
        .line_valid = line_follow->line_valid,
        .vision_valid = ball_balance->vision_valid,
    };
    output->line_follow_enabled = true;
    output->chassis.enabled = true;
    output->ball_balance.enabled = true;
}

Device_Status_e OLED_init_ex(void)
{
    oled_init_count++;
    return oled_init_status;
}

void OLED_operate_gram(pen_typedef pen)
{
    assert(pen == PEN_CLEAR);
    oled_clear_count++;
    memset(oled_rows, 0, sizeof(oled_rows));
}

void OLED_printf(uint8_t row, uint8_t col, const char *fmt, ...)
{
    assert(row < 5U);
    assert(col == 0U);
    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(oled_rows[row], sizeof(oled_rows[row]), fmt, args);
    va_end(args);
}

void OLED_refresh_gram(void)
{
    oled_refresh_count++;
}

BaseType_t xTaskCreate(TaskFunction_t task_code, const char *task_name,
    configSTACK_DEPTH_TYPE stack_depth, void *argument,
    UBaseType_t priority, TaskHandle_t *task_handle)
{
    (void) task_code;
    (void) task_name;
    (void) stack_depth;
    (void) argument;
    (void) priority;
    created_task_count++;
    if (task_handle != NULL) {
        *task_handle = (TaskHandle_t) 1;
    }
    return pdPASS;
}

TickType_t xTaskGetTickCount(void)
{
    return 0U;
}

void vTaskDelayUntil(TickType_t *last_wake_time, TickType_t period_ticks)
{
    (void) last_wake_time;
    (void) period_ticks;
}

int main(void)
{
    RobotInit();
    assert(created_task_count == 2U);
    assert(oled_init_count == 0U);

    RobotOLEDTask();
    assert(oled_init_count == 1U);
    assert(robot_oled_error_count == 1U);
    assert(!robot_oled_initialized);
    assert(oled_refresh_count == 0U);

    oled_init_status = DEVICE_OK;
    for (uint32_t i = 0U; i < 5U; i++) {
        RobotTask();
    }

    RobotOLEDTask();
    assert(robot_oled_initialized);
    assert(robot_oled_refresh_count == 1U);
    assert(oled_clear_count == 1U);
    assert(oled_refresh_count == 1U);
    assert(strcmp(oled_rows[0], "S:2 T:1s") == 0);
    assert(strcmp(oled_rows[1], "L:OK V:OK B:ON") == 0);
    assert(strcmp(oled_rows[2], "LT:   123 LM:   120") == 0);
    assert(strcmp(oled_rows[3], "RT:   130 RM:   127") == 0);
    assert(strcmp(oled_rows[4], "BALL:  42 ST:  321") == 0);
    return 0;
}
