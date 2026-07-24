/* 注意该文件应只用于任务初始化,只能被robot.c包含 */
#pragma once

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_dwt.h"
#include "bsp_log.h"
#include "framework_runtime.h"
#include "robot.h"

#include "hardware_test_config.h"

#if HARDWARE_TEST_MODE != HARDWARE_TEST_NONE
#include "hardware_test_task.h"
#endif

#if defined(ROBOT_ENABLE_INS_APP) && __has_include("ins.h")
#include "ins.h"
#define ROBOT_HAS_INS_TASK 1
#else
#define ROBOT_HAS_INS_TASK 0
#endif

#if __has_include("motor_task.h")
#include "motor_task.h"
#define ROBOT_HAS_MOTOR_TASK 1
#else
#define ROBOT_HAS_MOTOR_TASK 0
#endif

#if __has_include("referee_task.h")
#include "referee_task.h"
#define ROBOT_HAS_REFEREE_TASK 1
#else
#define ROBOT_HAS_REFEREE_TASK 0
#endif

#if __has_include("master_process.h")
#include "master_process.h"
#define ROBOT_HAS_MASTER_PROCESS 1
#else
#define ROBOT_HAS_MASTER_PROCESS 0
#endif

#if __has_include("daemon.h")
#include "daemon.h"
#define ROBOT_HAS_DAEMON_TASK 1
#else
#define ROBOT_HAS_DAEMON_TASK 0
#endif

#if __has_include("HT04.h")
#include "HT04.h"
#define ROBOT_HAS_HT04 1
#else
#define ROBOT_HAS_HT04 0
#endif

TaskHandle_t insTaskHandle;
TaskHandle_t robotTaskHandle;
TaskHandle_t motorTaskHandle;
TaskHandle_t daemonTaskHandle;
TaskHandle_t uiTaskHandle;
#if HARDWARE_TEST_MODE != HARDWARE_TEST_NONE
TaskHandle_t hardwareTestTaskHandle;
#endif

static void RobotCreateTask(TaskFunction_t task_code, const char *task_name,
    configSTACK_DEPTH_TYPE stack_depth, UBaseType_t priority,
    TaskHandle_t *task_handle)
{
    BaseType_t status = xTaskCreate(
        task_code, task_name, stack_depth, NULL, priority, task_handle);

    if (status != pdPASS) {
        LOGERROR("[freeRTOS] Failed to create task: %s", task_name);
        configASSERT(status == pdPASS);
    }
}

#if ROBOT_HAS_INS_TASK
void StartINSTASK(void *argument);
#endif
#if ROBOT_HAS_MOTOR_TASK
void StartMOTORTASK(void *argument);
#endif
#if ROBOT_HAS_DAEMON_TASK
void StartDAEMONTASK(void *argument);
#endif
void StartROBOTTASK(void *argument);
#if ROBOT_HAS_REFEREE_TASK
void StartUITASK(void *argument);
#endif

/**
 * @brief 初始化机器人任务,所有持续运行的任务都在这里初始化
 */
void OSTaskInit()
{
#if ROBOT_HAS_INS_TASK
    RobotCreateTask(StartINSTASK, "instask", 1024U,
        tskIDLE_PRIORITY + 3U, &insTaskHandle);
#endif

#if ROBOT_HAS_MOTOR_TASK
    RobotCreateTask(StartMOTORTASK, "motortask", 256U,
        tskIDLE_PRIORITY + 2U, &motorTaskHandle);
#endif

#if ROBOT_HAS_DAEMON_TASK
    RobotCreateTask(StartDAEMONTASK, "daemontask", 128U,
        tskIDLE_PRIORITY + 2U, &daemonTaskHandle);
#endif

    RobotCreateTask(StartROBOTTASK, "robottask", 1024U,
        tskIDLE_PRIORITY + 2U, &robotTaskHandle);

#if HARDWARE_TEST_MODE != HARDWARE_TEST_NONE
    RobotCreateTask(StartHARDWARETESTTASK, "hardwaretest", 256U,
        tskIDLE_PRIORITY + 1U, &hardwareTestTaskHandle);
#endif

#if ROBOT_HAS_REFEREE_TASK
    RobotCreateTask(StartUITASK, "uitask", 512U,
        tskIDLE_PRIORITY + 2U, &uiTaskHandle);
#endif

#if ROBOT_HAS_HT04
    HTMotorControlInit();
#endif
}

#if ROBOT_HAS_INS_TASK
__attribute__((noreturn)) void StartINSTASK(void *argument)
{
    (void) argument;

    if (!INS_Init()) {
        LOGERROR("[freeRTOS] INS initialization failed");
    }
    LOGINFO("[freeRTOS] INS Task Start");
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(1U);
    for (;;) {
        TickType_t start_tick = xTaskGetTickCount();
        INS_Task(0.001f);
#if ROBOT_HAS_MASTER_PROCESS
        VisionSend();
#endif
        TickType_t elapsed_ticks = xTaskGetTickCount() - start_tick;
        if (elapsed_ticks > period_ticks) {
            LOGERROR("[freeRTOS] INS Task delayed: %u ticks",
                (unsigned int) elapsed_ticks);
        }
        vTaskDelayUntil(&last_wake_time, period_ticks);
    }
}
#endif

#if ROBOT_HAS_MOTOR_TASK
__attribute__((noreturn)) void StartMOTORTASK(void *argument)
{
    (void) argument;

    LOGINFO("[freeRTOS] MOTOR Task Start");
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(1U);
    for (;;) {
        TickType_t start_tick = xTaskGetTickCount();
        MotorControlTask();
        TickType_t elapsed_ticks = xTaskGetTickCount() - start_tick;
        if (elapsed_ticks > period_ticks) {
            LOGERROR("[freeRTOS] MOTOR Task delayed: %u ticks",
                (unsigned int) elapsed_ticks);
        }
        vTaskDelayUntil(&last_wake_time, period_ticks);
    }
}
#endif

#if ROBOT_HAS_DAEMON_TASK
__attribute__((noreturn)) void StartDAEMONTASK(void *argument)
{
    (void) argument;

    LOGINFO("[freeRTOS] Daemon Task Start");
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(10U);
    for (;;) {
        TickType_t start_tick = xTaskGetTickCount();
#if ROBOT_HAS_DAEMON_TASK
        DaemonTask();
#endif
        TickType_t elapsed_ticks = xTaskGetTickCount() - start_tick;
        if (elapsed_ticks > period_ticks) {
            LOGERROR("[freeRTOS] Daemon Task delayed: %u ticks",
                (unsigned int) elapsed_ticks);
        }
        vTaskDelayUntil(&last_wake_time, period_ticks);
    }
}
#endif

__attribute__((noreturn)) void StartROBOTTASK(void *argument)
{
    (void) argument;

    framework_boot_stage = FRAMEWORK_BOOT_SCHEDULER_RUNNING;
    LOGINFO("[freeRTOS] ROBOT core Task Start");
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(5U);
    for (;;) {
        framework_robot_heartbeat++;
        TickType_t start_tick = xTaskGetTickCount();
        RobotTask();
        TickType_t elapsed_ticks = xTaskGetTickCount() - start_tick;
        if (elapsed_ticks > period_ticks) {
            LOGERROR("[freeRTOS] ROBOT core Task delayed: %u ticks",
                (unsigned int) elapsed_ticks);
        }
        vTaskDelayUntil(&last_wake_time, period_ticks);
    }
}

#if ROBOT_HAS_REFEREE_TASK
__attribute__((noreturn)) void StartUITASK(void *argument)
{
    (void) argument;

    LOGINFO("[freeRTOS] UI Task Start");
    MyUIInit();
    LOGINFO("[freeRTOS] UI Init Done, communication with ref has established");
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(1U);
    for (;;) {
        UITask();
        vTaskDelayUntil(&last_wake_time, period_ticks);
    }
}
#endif
