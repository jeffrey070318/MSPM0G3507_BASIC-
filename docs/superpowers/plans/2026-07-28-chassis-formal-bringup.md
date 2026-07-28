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
