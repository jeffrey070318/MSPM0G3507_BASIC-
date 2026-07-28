#include <assert.h>
#include <stddef.h>

#include "bsp_init.h"
#include "chassis.h"
#include "framework_runtime.h"
#include "imu.h"
#include "robot.h"
#include "task.h"
#include "vofa.h"

static unsigned bsp_init_count;
static unsigned imu_init_count;
static unsigned vofa_init_count;
static unsigned created_task_count;
static unsigned chassis_init_count;

volatile FrameworkBootStage_e framework_boot_stage;
volatile uint32_t framework_robot_heartbeat;
volatile uint32_t framework_main_heartbeat;

void BSPInit(void)
{
    bsp_init_count++;
}

void __disable_irq(void)
{
}

void __enable_irq(void)
{
}

void ChassisInit(void)
{
    chassis_init_count++;
}

void ChassisTask(void)
{
}

Device_Status_e IMU_Init(void)
{
    imu_init_count++;
    return DEVICE_OK;
}

Device_Status_e IMU_ReadAll(IMU_Data_t *data)
{
    (void) data;
    return DEVICE_ERROR;
}

Device_Status_e VOFA_Init(void)
{
    vofa_init_count++;
    return DEVICE_OK;
}

Device_Status_e VOFA_JustFloatOutputDMA(
    const float *channels, uint8_t channel_count)
{
    (void) channels;
    (void) channel_count;
    return DEVICE_OK;
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

void StartHARDWARETESTTASK(void *argument)
{
    (void) argument;
}

int main(void)
{
    RobotInit();
    assert(bsp_init_count == 1U);
    assert(vofa_init_count == 0U);
    assert(imu_init_count == 0U);
    assert(chassis_init_count == 0U);
    assert(created_task_count == 1U);
    return 0;
}
