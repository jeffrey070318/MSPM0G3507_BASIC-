#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ball_balance.h"
#include "bsp_init.h"
#include "chassis.h"
#include "competition.h"
#include "framework_runtime.h"
#include "imu.h"
#include "line_follow.h"
#include "oled.h"
#include "robot.h"
#include "task.h"
#include "vofa.h"

volatile FrameworkBootStage_e framework_boot_stage;
volatile uint32_t framework_robot_heartbeat;
volatile uint32_t framework_main_heartbeat;

static uint32_t chassis_init_count;
static uint32_t line_init_count;
static uint32_t balance_init_count;
static uint32_t competition_init_count;
static uint32_t chassis_task_count;
static uint32_t line_task_count;
static uint32_t balance_task_count;
static uint32_t competition_task_count;
static uint32_t imu_read_count;
static uint32_t vofa_output_count;
static uint32_t created_task_count;

void BSPInit(void)
{
}

void __disable_irq(void)
{
}

void __enable_irq(void)
{
}

bool ChassisInit(void)
{
    chassis_init_count++;
    return true;
}

void ChassisTask(const Chassis_Command_t *command,
    float dt_seconds, Chassis_Status_t *status)
{
    assert(command != NULL);
    assert(dt_seconds == 0.005f);
    assert(status != NULL);
    chassis_task_count++;
}

bool LineFollowInit(void)
{
    line_init_count++;
    return true;
}

void LineFollowTask(bool enabled, float dt_seconds,
    LineFollow_Output_t *output)
{
    (void) enabled;
    assert(dt_seconds == 0.005f);
    assert(output != NULL);
    output->line_valid = true;
    line_task_count++;
}

bool BallBalanceInit(void)
{
    balance_init_count++;
    return true;
}

void BallBalanceTask(const BallBalance_Command_t *command,
    uint32_t now_ms, float dt_seconds, BallBalance_Status_t *status)
{
    assert(command != NULL);
    assert(now_ms == balance_task_count + 1U);
    assert(dt_seconds == 0.001f);
    assert(status != NULL);
    balance_task_count++;
}

bool CompetitionInit(void)
{
    competition_init_count++;
    return true;
}

void CompetitionTask(uint32_t now_ms, bool app_ready,
    const LineFollow_Output_t *line_follow,
    const BallBalance_Status_t *ball_balance,
    Competition_Output_t *output)
{
    assert(now_ms == 5U);
    assert(app_ready);
    assert(line_follow != NULL);
    assert(ball_balance != NULL);
    assert(output != NULL);
    output->line_follow_enabled = true;
    competition_task_count++;
}

Device_Status_e IMU_Init(void)
{
    return DEVICE_OK;
}

Device_Status_e IMU_ReadAll(IMU_Data_t *data)
{
    assert(data != NULL);
    imu_read_count++;
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
    vofa_output_count++;
    return DEVICE_OK;
}

Device_Status_e OLED_init_ex(void)
{
    return DEVICE_OK;
}

void OLED_operate_gram(pen_typedef pen)
{
    (void) pen;
}

void OLED_printf(uint8_t row, uint8_t col, const char *fmt, ...)
{
    (void) row;
    (void) col;
    (void) fmt;
}

void OLED_refresh_gram(void)
{
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
    assert(chassis_init_count == 1U);
    assert(line_init_count == 1U);
    assert(balance_init_count == 1U);
    assert(competition_init_count == 1U);
    assert(created_task_count == 2U);

    for (uint32_t i = 0U; i < 4U; i++) {
        RobotTask();
    }
    assert(balance_task_count == 4U);
    assert(line_task_count == 0U);
    assert(competition_task_count == 0U);
    assert(chassis_task_count == 0U);
    assert(imu_read_count == 0U);

    RobotTask();
    assert(balance_task_count == 5U);
    assert(line_task_count == 1U);
    assert(competition_task_count == 1U);
    assert(chassis_task_count == 1U);
    assert(imu_read_count == 1U);
    assert(vofa_output_count == 0U);
    return 0;
}
