#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "bsp_init.h"
#include "chassis.h"
#include "framework_runtime.h"
#include "imu.h"
#include "motor.h"
#include "oled.h"
#include "robot.h"
#include "task.h"
#include "vofa.h"

extern volatile bool robot_oled_initialized;
extern volatile uint32_t robot_oled_refresh_count;
extern volatile uint32_t robot_oled_error_count;
void RobotOLEDTask(void);

Motor_Device_t chassis_motors[2];
volatile bool chassis_manual_enabled;
volatile float chassis_manual_vx_mps;
volatile float chassis_manual_wz_radps;

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

void ChassisInit(void)
{
}

void ChassisTask(void)
{
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
    chassis_manual_enabled = true;
    chassis_manual_vx_mps = 0.05f;
    chassis_manual_wz_radps = -0.10f;
    chassis_motors[0].target_speed = 123.0f;
    chassis_motors[0].measured_speed = 120.0f;
    chassis_motors[0].control_output = 0.25f;
    chassis_motors[1].target_speed = 130.0f;
    chassis_motors[1].measured_speed = 127.0f;
    chassis_motors[1].control_output = -0.30f;

    RobotOLEDTask();
    assert(robot_oled_initialized);
    assert(robot_oled_refresh_count == 1U);
    assert(oled_clear_count == 1U);
    assert(oled_refresh_count == 1U);
    assert(strcmp(oled_rows[0], "M:OFF CMD:ON") == 0);
    assert(strcmp(oled_rows[1], "LT:   123 LM:   120") == 0);
    assert(strcmp(oled_rows[2], "RT:   130 RM:   127") == 0);
    assert(strcmp(oled_rows[3], "LO: 250 RO:-300") == 0);
    assert(strcmp(oled_rows[4], "V:   50 W: -100") == 0);
    return 0;
}
