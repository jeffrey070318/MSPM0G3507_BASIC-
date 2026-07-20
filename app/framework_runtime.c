#include "framework_runtime.h"

volatile FrameworkBootStage_e framework_boot_stage = FRAMEWORK_BOOT_RESET;
volatile uint32_t framework_robot_heartbeat;
volatile uint32_t framework_main_heartbeat;
