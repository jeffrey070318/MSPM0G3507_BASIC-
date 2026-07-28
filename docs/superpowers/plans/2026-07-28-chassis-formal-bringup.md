# Chassis Formal Bring-up Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enable the two-wheel chassis app with two directly registered DRV8701E motors and a safe, explicit manual command entry for hardware bring-up.

**Architecture:** `app/chassis` remains the composition point for the two concrete motor instances. The generic Motor module is unchanged; chassis uses SysConfig-generated GPIO aliases, binds the left/right encoders, and defaults to zero force until an explicit manual command is enabled.

**Tech Stack:** C11, MSPM0 DriverLib/SysConfig aliases, FreeRTOS application task, GCC host tests, CMake/Ninja.

---

### Task 1: Lock Direct Motor Registration and Safe Command Behavior

**Files:**
- Create: `tests/chassis_stubs/bsp_dwt.h`
- Create: `tests/chassis_stubs/motor.h`
- Create: `tests/chassis_stubs/ti_msp_dl_config.h`
- Create: `tests/chassis_registration_test.c`
- Modify: `tests/run_host_tests.ps1`
- Modify: `app/chassis/chassis.h`
- Modify: `app/chassis/chassis.c`

- [x] Write a host test which verifies that `ChassisInit()` registers two DRV8701E motors with `htim1/htim2`, generated AIN1/BIN1 aliases, left/right encoders, and synchronized driver/encoder reverse values.
- [x] Verify the test fails because the manual command API and generated GPIO aliases are not yet used.
- [x] Replace raw phase GPIO definitions with `MOTOR_GPIO_AIN1/BIN1_PORT/PIN` and add `ChassisSetManualCommand()` plus `ChassisDisableManualCommand()`.
- [x] Make the no-INS path remain in `CHASSIS_ZERO_FORCE` until manual control is explicitly enabled; remove the fixed `vx=0.3f` fallback.
- [x] Verify the new test passes and run all host tests.

### Task 2: Enable and Document Formal Bring-up

**Files:**
- Modify: `app/robot_def.h`
- Modify: `app/chassis/chassis.md`
- Modify: `框架使用说明.md`

- [x] Enable `ROBOT_ENABLE_CHASSIS_APP` while leaving hardware-test mode at `HARDWARE_TEST_NONE`.
- [x] Document the safe startup state, explicit manual command API, wheel-off-ground first run, and remaining right-wheel direction validation.
- [x] Build FreeRTOS, NoRTOS, and BSP/module link-check targets; run the SysConfig static checker.
- [x] Review the final diff and commit only on the feature branch.

### Task 3: Centralize Chassis Tuning Parameters

**Files:**
- Modify: `app/robot_def.h`
- Modify: `app/chassis/chassis.c`
- Modify: `module/ins/ins.h`
- Modify: `tests/chassis_registration_test.c`

- [x] Move chassis mechanical, encoder, transmission, speed PID, limit, and direction parameters into clearly commented sections in `robot_def.h`.
- [x] Make chassis registration, kinematics, and INS odometry consume the shared parameters.
- [x] Verify PID registration and target-speed conversion through the chassis host test.

### Task 4: Simplify Chassis Source Layout

**Files:**
- Modify: `app/chassis/chassis.c`
- Modify: `app/chassis/chassis.h`

- [x] Remove left/right pin macros and register both DRV8701E instances explicitly in `ChassisInit()`.
- [x] Keep runtime state as a small set of directly named variables without adding another abstraction layer.
- [x] Separate command selection, differential kinematics, motor application, and feedback publication into focused functions.
- [x] Preserve the public API, Live Watch symbols, and tested runtime behavior.

### Task 5: Integrate the Normal-Mode OLED Display

**Files:**
- Modify: `app/robot.c`
- Modify: `app/robot.h`
- Modify: `app/robot_task.h`
- Create: `tests/robot_oled_test.c`

- [x] Test normal-mode OLED task creation, initialization retry, and five display rows.
- [x] Render chassis state, target/measured speeds, outputs, and manual velocity command from `robot.c`.
- [x] Refresh in a generic low-priority 5 Hz OLED task so blocking I2C does not delay chassis control.
- [x] Keep hardware-test mode isolated and verify all host and firmware builds.
