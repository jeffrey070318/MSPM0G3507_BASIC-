# Static LED And Buzzer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add caller-owned LED/key/servo modules, a fixed active-buzzer module, and remove runtime resource unregistration.

**Architecture:** SysConfig remains the hardware source of truth. Modules acquire GPIO/PWM BSP instances once during boot and retain them until reset; runtime APIs only operate the device.

**Tech Stack:** C11, TI MSPM0 DriverLib compatibility BSP, CMake/GCC, PowerShell host-test runner.

---

### Task 1: Lock Indicator Behavior With Host Tests

**Files:**
- Create: `tests/indicator_test.c`
- Create: `tests/indicator_stubs/ti_msp_dl_config.h`
- Modify: `tests/run_host_tests.ps1`

- [ ] Add tests for LED and buzzer initialization, active-high on/off/toggle, null or unbound LED objects, and failed registration.
- [ ] Run `tests/run_host_tests.ps1` and confirm the indicator test fails because the module headers or sources do not exist.

### Task 2: Add Minimal LED And Buzzer Modules

**Files:**
- Create: `module/led/led.h`
- Create: `module/led/led.c`
- Create: `module/buzzer/buzzer.h`
- Create: `module/buzzer/buzzer.c`
- Modify: `module/module.md`
- Modify: `app/robot_task.h`

- [ ] Implement caller-owned LED instances, fixed-buzzer registration, and `On`, `Off`, and `Toggle` operations.
- [ ] Remove the legacy optional buzzer-task detection from `robot_task.h`.
- [ ] Run the indicator host test and confirm it passes.

### Task 3: Remove Runtime Unregistration

**Files:**
- Modify: `bsp/gpio/bsp_gpio.h`
- Modify: `bsp/gpio/bsp_gpio.c`
- Modify: `bsp/pwm/bsp_pwm.h`
- Modify: `bsp/pwm/bsp_pwm.c`
- Modify: `module/motor/motor.h`
- Modify: `module/motor/motor.c`
- Modify: `module/motor/driver/motor_driver.h`
- Modify: `module/motor/driver/motor_driver.c`
- Modify: `module/motor/driver/drv8701e_driver.c`
- Modify: `module/motor/driver/tb6612_driver.c`
- Modify: `module/gray_sensor/gray_sensor.c`
- Modify: `app/chassis/chassis.c`
- Modify: `tests/bsp_gpio_test.c`
- Modify: `tests/bsp_pwm_shared_test.c`
- Modify: `tests/motor_driver_test.c`
- Modify: `tests/motor_closed_loop_test.c`
- Modify: `tests/motor_stubs/bsp_gpio.h`
- Modify: `tests/motor_stubs/bsp_pwm.h`

- [ ] Delete unregister/deinit declarations, implementations, callbacks, rollback calls, and cleanup assertions.
- [ ] Keep initialization failure returns and duplicate resource checks intact.

### Task 4: Add A Minimal Key Module

**Files:**
- Create: `module/key/key.h`
- Create: `module/key/key.c`
- Create: `tests/key_test.c`
- Modify: `tests/run_host_tests.ps1`
- Modify: `module/module.md`

- [ ] Test caller-owned key binding, active-high and active-low reads, null objects, invalid configuration, and registration failure.
- [ ] Implement `KEY_Init()` and immediate `KEY_IsPressed()` reads without debounce or task logic.

### Task 5: Add A Minimal Servo Module

**Files:**
- Create: `module/servo/servo.h`
- Create: `module/servo/servo.c`
- Create: `tests/servo_test.c`
- Modify: `tests/run_host_tests.ps1`
- Modify: `module/module.md`

- [ ] Test PWM registration, 0--180 degree clamping, pulse conversion, stop behavior, invalid configuration, and registration failure.
- [ ] Implement a caller-owned servo with configurable pulse limits and a fixed 20 ms period.

### Task 6: Verify The Complete Firmware

- [ ] Run `tests/run_host_tests.ps1` and require all host tests to pass.
- [ ] Run `cmake --build build --target all`.
- [ ] Run `cmake --build build-link-check --target all`.
- [ ] Search for removed API names and require zero references.
- [ ] Run `git diff --check`.
