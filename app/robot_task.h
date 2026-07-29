/* This file defines task entry points and is included only by robot.c. */
#pragma once

#include "FreeRTOS.h"
#include "task.h"

#include "framework_runtime.h"
#include "hardware_test_config.h"
#include "robot.h"

#if HARDWARE_TEST_MODE != HARDWARE_TEST_NONE
#include "hardware_test_task.h"
#endif

#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
static TaskHandle_t robotTaskHandle;
static TaskHandle_t oledTaskHandle;
#else
static TaskHandle_t hardwareTestTaskHandle;
#endif

static void RobotCreateTask(TaskFunction_t task_code, const char *task_name,
    configSTACK_DEPTH_TYPE stack_depth, UBaseType_t priority,
    TaskHandle_t *task_handle)
{
    BaseType_t status = xTaskCreate(
        task_code, task_name, stack_depth, NULL, priority, task_handle);
    configASSERT(status == pdPASS);
}

#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
static void StartRobotTask(void *argument);
static void StartOLEDTask(void *argument);
#endif

static void OSTaskInit(void)
{
#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
    RobotCreateTask(StartRobotTask, "robot", 1024U,
        tskIDLE_PRIORITY + 2U, &robotTaskHandle);
    RobotCreateTask(StartOLEDTask, "oled", 256U,
        tskIDLE_PRIORITY + 1U, &oledTaskHandle);
#else
    RobotCreateTask(StartHARDWARETESTTASK, "hardwaretest", 256U,
        tskIDLE_PRIORITY + 1U, &hardwareTestTaskHandle);
#endif
}

#if HARDWARE_TEST_MODE == HARDWARE_TEST_NONE
__attribute__((noreturn)) static void StartRobotTask(void *argument)
{
    (void) argument;
    framework_boot_stage = FRAMEWORK_BOOT_SCHEDULER_RUNNING;

    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(1U);
    for (;;) {
        framework_robot_heartbeat++;
        RobotTask();
        vTaskDelayUntil(&last_wake_time, period_ticks);
    }
}

__attribute__((noreturn)) static void StartOLEDTask(void *argument)
{
    (void) argument;

    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(200U);
    for (;;) {
        RobotOLEDTask();
        vTaskDelayUntil(&last_wake_time, period_ticks);
    }
}
#endif
