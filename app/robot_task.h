/* 注意该文件应只用于任务初始化,只能被robot.c包含 */
#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "bsp_dwt.h"
#include "bsp_log.h"
#include "robot.h"

#if __has_include("ins_task.h")
#include "ins_task.h"
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

#if __has_include("buzzer.h")
#include "buzzer.h"
#define ROBOT_HAS_BUZZER 1
#else
#define ROBOT_HAS_BUZZER 0
#endif

osThreadId insTaskHandle;
osThreadId robotTaskHandle;
osThreadId motorTaskHandle;
osThreadId daemonTaskHandle;
osThreadId uiTaskHandle;

#if ROBOT_HAS_INS_TASK
void StartINSTASK(void const *argument);
#endif
#if ROBOT_HAS_MOTOR_TASK
void StartMOTORTASK(void const *argument);
#endif
#if ROBOT_HAS_DAEMON_TASK || ROBOT_HAS_BUZZER
void StartDAEMONTASK(void const *argument);
#endif
void StartROBOTTASK(void const *argument);
#if ROBOT_HAS_REFEREE_TASK
void StartUITASK(void const *argument);
#endif

/**
 * @brief 初始化机器人任务,所有持续运行的任务都在这里初始化
 */
void OSTaskInit()
{
#if ROBOT_HAS_INS_TASK
    osThreadDef(instask, StartINSTASK, osPriorityAboveNormal, 0, 1024);
    insTaskHandle = osThreadCreate(osThread(instask), NULL);
#endif

#if ROBOT_HAS_MOTOR_TASK
    osThreadDef(motortask, StartMOTORTASK, osPriorityNormal, 0, 256);
    motorTaskHandle = osThreadCreate(osThread(motortask), NULL);
#endif

#if ROBOT_HAS_DAEMON_TASK || ROBOT_HAS_BUZZER
    osThreadDef(daemontask, StartDAEMONTASK, osPriorityNormal, 0, 128);
    daemonTaskHandle = osThreadCreate(osThread(daemontask), NULL);
#endif

    osThreadDef(robottask, StartROBOTTASK, osPriorityNormal, 0, 1024);
    robotTaskHandle = osThreadCreate(osThread(robottask), NULL);

#if ROBOT_HAS_REFEREE_TASK
    osThreadDef(uitask, StartUITASK, osPriorityNormal, 0, 512);
    uiTaskHandle = osThreadCreate(osThread(uitask), NULL);
#endif

#if ROBOT_HAS_HT04
    HTMotorControlInit();
#endif
}

#if ROBOT_HAS_INS_TASK
__attribute__((noreturn)) void StartINSTASK(void const *argument)
{
    (void) argument;
    static float ins_start;
    static float ins_dt;

    INS_Init();
    LOGINFO("[freeRTOS] INS Task Start");
    for (;;) {
        ins_start = DWT_GetTimeline_ms();
        INS_Task();
        ins_dt = DWT_GetTimeline_ms() - ins_start;
        if (ins_dt > 1.0f) {
            LOGERROR("[freeRTOS] INS Task is being DELAY! dt = [%f]", &ins_dt);
        }
#if ROBOT_HAS_MASTER_PROCESS
        VisionSend();
#endif
        osDelay(1);
    }
}
#endif

#if ROBOT_HAS_MOTOR_TASK
__attribute__((noreturn)) void StartMOTORTASK(void const *argument)
{
    (void) argument;
    static float motor_dt;
    static float motor_start;

    LOGINFO("[freeRTOS] MOTOR Task Start");
    for (;;) {
        motor_start = DWT_GetTimeline_ms();
        MotorControlTask();
        motor_dt = DWT_GetTimeline_ms() - motor_start;
        if (motor_dt > 1.0f) {
            LOGERROR("[freeRTOS] MOTOR Task is being DELAY! dt = [%f]", &motor_dt);
        }
        osDelay(1);
    }
}
#endif

#if ROBOT_HAS_DAEMON_TASK || ROBOT_HAS_BUZZER
__attribute__((noreturn)) void StartDAEMONTASK(void const *argument)
{
    (void) argument;
    static float daemon_dt;
    static float daemon_start;

#if ROBOT_HAS_BUZZER
    BuzzerInit();
#endif
    LOGINFO("[freeRTOS] Daemon Task Start");
    for (;;) {
        daemon_start = DWT_GetTimeline_ms();
#if ROBOT_HAS_DAEMON_TASK
        DaemonTask();
#endif
#if ROBOT_HAS_BUZZER
        BuzzerTask();
#endif
        daemon_dt = DWT_GetTimeline_ms() - daemon_start;
        if (daemon_dt > 10.0f) {
            LOGERROR("[freeRTOS] Daemon Task is being DELAY! dt = [%f]", &daemon_dt);
        }
        osDelay(10);
    }
}
#endif

__attribute__((noreturn)) void StartROBOTTASK(void const *argument)
{
    (void) argument;
    static float robot_dt;
    static float robot_start;

    LOGINFO("[freeRTOS] ROBOT core Task Start");
    for (;;) {
        robot_start = DWT_GetTimeline_ms();
        RobotTask();
        robot_dt = DWT_GetTimeline_ms() - robot_start;
        if (robot_dt > 5.0f) {
            LOGERROR("[freeRTOS] ROBOT core Task is being DELAY! dt = [%f]", &robot_dt);
        }
        osDelay(5);
    }
}

#if ROBOT_HAS_REFEREE_TASK
__attribute__((noreturn)) void StartUITASK(void const *argument)
{
    (void) argument;

    LOGINFO("[freeRTOS] UI Task Start");
    MyUIInit();
    LOGINFO("[freeRTOS] UI Init Done, communication with ref has established");
    for (;;) {
        UITask();
        osDelay(1);
    }
}
#endif
