# Motor Closed-Loop Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an app-owned closed-loop brushed DC motor module with embedded speed PID and interchangeable DRV8701E/TB6612 power-stage backends.

**Architecture:** `module/algorithm` owns a platform-independent PID. `module/motor` owns one caller-supplied motor object, registers and releases its BSP PWM/GPIO resources, binds one existing BSP encoder, and dispatches normalized output through a tagged backend. No global motor registry or dynamic motor allocation is introduced.

**Tech Stack:** C11 host tests, C99 MSPM0 firmware, DriverLib-backed BSP, CMake/GCC.

---

### Task 1: Shared PWM timer behavior

**Files:**
- Modify: `bsp/pwm/bsp_pwm.c`
- Modify: `bsp/pwm/bsp_pwm.h`
- Modify: `tests/bsp_pwm_shared_test.c`

- [ ] Add host assertions that stopping one shared-timer channel leaves its peer running and stopping the final channel stops the timer.
- [ ] Run `tests/run_host_tests.ps1` and confirm the new shared-timer assertion fails against the old PWM behavior.
- [ ] Implement per-channel stop handling and last-running-channel timer shutdown.
- [ ] Re-run the host tests and confirm the shared-timer test passes.

### Task 2: Platform-independent speed PID

**Files:**
- Create: `module/algorithm/pid.c`
- Create: `module/algorithm/pid.h`
- Create: `tests/pid_controller_test.c`
- Modify: `tests/run_host_tests.ps1`

- [ ] Add host tests for proportional response, explicit-time integral accumulation, output/integral limiting, conditional anti-windup, derivative-on-measurement, deadband, reset, and invalid `dt`.
- [ ] Run the PID test and confirm it fails because the new API is absent.
- [ ] Implement `PID_ControllerInit`, `PID_ControllerUpdate`, `PID_ControllerReset`, and `PID_ControllerSetTunings` without DWT or CMSIS dependencies.
- [ ] Re-run the PID and existing host tests.

### Task 3: DRV8701E and TB6612 backends

**Files:**
- Create: `module/motor/driver/motor_driver.c`
- Create: `module/motor/driver/motor_driver.h`
- Create: `module/motor/driver/drv8701e_driver.c`
- Create: `module/motor/driver/tb6612_driver.c`
- Create: `tests/motor_driver_test.c`
- Create: `tests/stubs/bsp_encoder.h`
- Create: `tests/stubs/bsp_gpio.h`
- Create: `tests/stubs/bsp_pwm.h`
- Modify: `tests/run_host_tests.ps1`

- [ ] Add host tests proving DRV8701E uses EN/PWM plus PH/DIR, zero output brakes, and direction changes pass through zero duty.
- [ ] Add host tests proving TB6612 supports PWM plus IN1/IN2 and configurable coast/brake behavior.
- [ ] Add failure-injection tests proving GPIO/PWM registration failure is reported.
- [ ] Run the driver test and confirm it fails because the backend API is absent.
- [ ] Implement tagged initialization with function-pointer runtime dispatch.
- [ ] Re-run all host tests.

### Task 4: Closed-loop motor aggregation and chassis migration

**Files:**
- Modify: `module/motor/motor.c`
- Modify: `module/motor/motor.h`
- Create: `tests/motor_closed_loop_test.c`
- Modify: `app/chassis/chassis.c`
- Modify: `tests/run_host_tests.ps1`

- [ ] Add host tests for one-call initialization, encoder binding, counts-per-second feedback, embedded PID output, open-loop output, stop/enable, and invalid `dt`.
- [ ] Run the motor test and confirm it fails against the old API.
- [ ] Implement caller-owned `Motor_Device_t`, `Motor_Init`, `Motor_SetOpenLoop`, `Motor_SetTargetSpeed`, `Motor_Update`, `Motor_Enable`, and `Motor_Stop`.
- [ ] Migrate chassis to two static DRV8701E motor instances, use AIN1/BIN1 as PH, leave AIN2/BIN2 unused, and pass app-measured DWT `dt` into each update.
- [ ] Convert chassis target velocity from counts-per-control-period to counts per second.

### Task 5: Documentation and full verification

**Files:**
- Create: `module/algorithm/algorithm.md`
- Create: `module/motor/motor.md`
- Modify: `module/module.md`

- [ ] Document ownership, units, DRV8701E braking semantics, TB6612 stop modes, and initialization examples.
- [ ] Run `tests/run_host_tests.ps1`.
- [ ] Run `cmake --build build --target all`.
- [ ] Run `cmake --build build-link-check --target all`.
- [ ] Run the MSPM0 SysConfig static checker and `git diff --check`.
