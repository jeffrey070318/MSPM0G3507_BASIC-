#ifndef FRAMEWORK_RUNTIME_H
#define FRAMEWORK_RUNTIME_H

#include <stdint.h>

typedef enum {
    FRAMEWORK_BOOT_RESET = 0,
    FRAMEWORK_BOOT_SYSCFG_READY,
    FRAMEWORK_BOOT_ROBOT_READY,
    FRAMEWORK_BOOT_SCHEDULER_RUNNING,
    FRAMEWORK_BOOT_SCHEDULER_FAILED,
    FRAMEWORK_BOOT_NORTOS_RUNNING,
} FrameworkBootStage_e;

extern volatile FrameworkBootStage_e framework_boot_stage;
extern volatile uint32_t framework_robot_heartbeat;
extern volatile uint32_t framework_main_heartbeat;

#endif
